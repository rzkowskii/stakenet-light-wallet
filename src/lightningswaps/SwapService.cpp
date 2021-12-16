#include "SwapService.hpp"
#include <Orderbook/OrderbookBestPriceModel.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/OrderbookSwapPeerPool.hpp>
#include <Orderbook/TradingOrdersModel.hpp>
#include <Service/ChannelRentingManager.hpp>
#include <Service/PaymentNodeRegistrationService.hpp>
#include <Swaps/AbstractSwapClient.hpp>
#include <Swaps/LndSwapClient.hpp>
#include <Swaps/SwapClientPool.hpp>
#include <Swaps/SwapManager.hpp>
#include <Swaps/SwapPeerPool.hpp>
#include <Swaps/SwapRepository.hpp>
#include <Utils/GenericProtoDatabase.hpp>
#include <Utils/Logging.hpp>
#include <tinyformat.h>

#include <QDir>

namespace swaps {

static constexpr uint32_t ORDERBOOK_MIN_API_VERSION = 2;

static SwapService::State ConvertState(orderbook::OrderbookApiClient::State state)
{
    switch (state) {
    case orderbook::OrderbookApiClient::State::Connected:
        return SwapService::State::Connected;
    case orderbook::OrderbookApiClient::State::InMaintenance:
        return SwapService::State::InMaintenance;
    case orderbook::OrderbookApiClient::State::Disconnected:
    case orderbook::OrderbookApiClient::State::Error:
        return SwapService::State::Disconnected;
    }

    return SwapService::State::Disconnected;
}

//==============================================================================

SwapService::SwapService(Config cfg, QObject* parent)
    : QObject(parent)
    , _cfg(cfg)
{
    init();
}

//==============================================================================

SwapService::~SwapService() {}

//==============================================================================

void SwapService::start()
{
    QMetaObject::invokeMethod(this, [this] { _orderbook->start(); });
}

//==============================================================================

void SwapService::stop() {}

//==============================================================================

Promise<void> SwapService::addCurrency(std::string currency)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            try {
                auto listCurrencies = _clientsPool->activeClients();
                auto it = std::find(std::begin(listCurrencies), std::end(listCurrencies), currency);
                if (it == listCurrencies.end()) {
                    // TODO(yuraolex): it's not enough info to pass only currency, need to extend
                    // this to support both currency and token

                    // TODO: replace this check
                    if (currency == "WETH" || currency == "USDT" || currency == "USDC"
                        || currency == "ETH") {
                        _clientsPool->addConnextClient(currency);
                    } else {
                        _clientsPool->addLndClient(currency);
                    }
                }
                resolve();
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<std::vector<orderbook::TradingPair>> SwapService::tradingPairs()
{
    return Promise<std::vector<orderbook::TradingPair>>(
        [=](const auto& resolve, const auto& reject) {
            QMetaObject::invokeMethod(this, [=] {
                _orderbook->tradingPairs()
                    .then([this, resolve](orderbook::OrderbookClient::TradingPairInfo info) {
                        auto paysFees = std::get<1>(info);
                        if (paysFees) {
                            _cfg.payFee = paysFees;
                        }
                        resolve(std::get<0>(info));
                    })
                    .fail(reject);
            });
        });
}

//==============================================================================

Promise<void> SwapService::activatePair(std::string pairId)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            try {
                _orderbook->subscribe(pairId);
                resolve();
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<void> SwapService::deactivatePair(std::string pairId)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            try {
                _orderbook->unsubscribe(pairId);
                _orderbook->cancelOrders(pairId);
                resolve();
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<std::vector<std::string>> SwapService::activatedPairs()
{
    return Promise<std::vector<std::string>>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(this, [=] { resolve(_orderbook->activatedPairs()); });
    });
}

//==============================================================================

Promise<std::vector<orderbook::LimitOrder>> SwapService::listOrders(
    std::string pairId, int64_t lastKnownPrice, uint32_t limit)
{
    limit = std::min<uint32_t>(limit, 100);
    return Promise<std::vector<orderbook::LimitOrder>>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(this, [=] {
            std::vector<orderbook::LimitOrder> result;

            auto traverser = [&](bool isBuy, auto details) {
                result.emplace_back(orderbook::LimitOrder{
                    orderbook::MarketOrder{ details.amount, pairId, isBuy }, details.price });
            };
            _orderbook->ordersModel()->traverseAsks(
                pairId, lastKnownPrice, limit, std::bind(traverser, false, std::placeholders::_1));
            _orderbook->ordersModel()->traverseBids(
                pairId, lastKnownPrice, limit, std::bind(traverser, true, std::placeholders::_1));

            resolve(result);
        });
    });
}

//==============================================================================

Promise<std::vector<orderbook::OwnOrder>> SwapService::listOwnOrders(std::string pairId)
{
    return Promise<std::vector<orderbook::OwnOrder>>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(this, [=] {
            std::vector<orderbook::OwnOrder> result;

            auto traverser = [&](auto ownOrder) { result.emplace_back(ownOrder); };

            _orderbook->ownOrdersByPairId(traverser, pairId);

            resolve(result);
        });
    });
}

//==============================================================================

Promise<std::vector<std::string>> SwapService::listCurrencies()
{
    return Promise<std::vector<std::string>>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(this, [=] { resolve(_clientsPool->activeClients()); });
    });
}

//==============================================================================

Promise<PlaceOrderResult> SwapService::placeOrder(orderbook::LimitOrder ownLimitOrder)
{
    return Promise<PlaceOrderResult>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=]() {
            this->executeBaseOrderChecks(ownLimitOrder)
                .then([this, ownLimitOrder] {
                    return this->executePlaceOrderFeePayment(ownLimitOrder)
                        .then([this, ownLimitOrder](storage::RefundableFee refundableFee) {
                            return this->executePlaceOrder(ownLimitOrder, refundableFee);
                        });
                })
                .then([resolve](PlaceOrderResult result) { resolve(result); })
                .fail([reject](SwapFailure failure) { reject(PlaceOrderFailure(failure)); })
                .fail([reject](FeeFailure failure) { reject(PlaceOrderFailure(failure)); })
                .fail([reject]() { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

Promise<void> SwapService::cancelOrder(std::string pairId, std::string localId)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            _orderbook->cancelOrder(pairId, localId)
                .then([localId, this] { return Promise<void>::resolve(); })
                .tapFail([localId](const std::exception& ex) {
                    LogCCritical(Orderbook)
                        << "Failed to refund fee when canceling order:" << localId.c_str()
                        << ex.what();
                })
                .then([resolve]() { resolve(); })
                .fail([reject]() { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

void SwapService::cancelAllOrders(std::string pairId)
{
    QMetaObject::invokeMethod(this, [=] {
        std::vector<std::string> ids;
        _orderbook->ownOrdersByPairId(
            [&](const auto& order) { ids.emplace_back(order.localId); }, pairId);
        std::for_each(std::begin(ids), std::end(ids),
            [pairId, this](auto localId) { _orderbook->cancelOrder(pairId, localId); });
    });
}

//==============================================================================

orderbook::OrderbookClient* SwapService::orderBookClient() const
{
    return _orderbook;
}

//==============================================================================

AbstractSwapRepository* SwapService::swapRepository() const
{
    return _repository;
}

//==============================================================================

RefundableFeeManager* SwapService::feeRefunding() const
{
    return _refundableFees;
}

//==============================================================================

orderbook::ChannelRentingManager* SwapService::channelRentingManager() const
{
    return _rentingManager;
}

//==============================================================================

void SwapService::onCheckState()
{
    if (_orderbook->state() == orderbook::OrderbookApiClient::State::Connected) {
        tradingPairs().then([this](std::vector<orderbook::TradingPair> pairs) {
            _tradingPairs.resize(pairs.size());
            std::transform(
                std::begin(pairs), std::end(pairs), std::begin(_tradingPairs), [](const auto& tp) {
                    orderbook::TradingPair result;
                    result.pairId = tp.pairId;
                    result.buyFeePercent = tp.buyFeePercent;
                    result.sellFeePercent = tp.sellFeePercent;
                    result.buyFundsInterval = tp.buyFundsInterval;
                    result.buyPriceInterval = tp.buyPriceInterval;
                    result.sellFundsInterval = tp.sellFundsInterval;
                    result.sellPriceInterval = tp.sellPriceInterval;
                    return result;
                });
        });
    }

    stateChanged(ConvertState(_orderbook->state()));
}

//==============================================================================

void SwapService::onSwapSuccess(const SwapSuccess& swap, const storage::RefundableFee& fee)
{
    if (auto matchedTrade = _orderbook->tryGetMatchedTrade(swap.pairId, swap.orderId)) {
        auto ownOrder = std::get<1>(*matchedTrade).get();
        auto quantityDiff
            = ownOrder.quantity - (ownOrder.isBuy ? swap.amountReceived : swap.amountSent);

        _orderbook->onOwnOrderSwapSuccess(swap);
        auto baseOrder = _orderbook->tryGetBaseOrder(swap.pairId, swap.orderId);

        if (quantityDiff > 0) {
            auto amounts = swaps::MakerTakerAmounts::CalculateMakerTakerAmounts(
                quantityDiff, ownOrder.price, ownOrder.isBuy, ownOrder.pairId);
            if (amounts.taker.units > 0 && amounts.maker.units > 0) {
                auto ownMakerOrder = _orderbook->tryGetOwnOrder(swap.pairId, swap.localId);

                const auto quantity = amounts.taker.amount;
                if (baseOrder || ownMakerOrder) {
                    auto order = baseOrder ? baseOrder : ownMakerOrder;

                    placePartialOrder(order.get(), quantity, fee)
                        .then([order = ownOrder, quantity]() {
                            LogCCInfo(Swaps)
                                << QString("Swap succeeded: partial order was successfully posted: "
                                           "isBuy-%1, price-%2, quantity-%3")
                                       .arg(order.isBuy)
                                       .arg(order.price)
                                       .arg(quantity);
                        })
                        .fail([this, order, quantity, fee](const std::exception& ex) mutable {
                            QString excep(ex.what());
                            if (excep.contains(
                                    "There aren't orders to fulfill your market order")) {
                                order->orderType = orderbook::OwnOrderType::Limit;
                                placePartialOrder(order.get(), quantity, fee)
                                    .then([order, quantity]() {
                                        LogCCInfo(Swaps)
                                            << QString("Changed market to limit partial order was "
                                                       "successfully posted : "
                                                       "isBuy-%1, price-%2, quantity-%3")
                                                   .arg(order->isBuy)
                                                   .arg(order->price)
                                                   .arg(quantity);
                                    })
                                    .fail([](const std::exception& ex) {
                                        LogCCritical(Swaps) << "Failed to repost changed market to "
                                                               "limit partial order : "
                                                            << ex.what();
                                    });
                            }

                            LogCCritical(Swaps)
                                << "Swap succeeded: Failed to post partial order" << ex.what();
                        });
                } else {
                    auto limitOrder = ownOrder;
                    limitOrder.quantity = quantity;
                    placeOrder(limitOrder, fee)
                        .then([ownOrder = limitOrder] {
                            LogCCInfo(Swaps) << QString("Swap succeeded: order was successfully "
                                                        "reposted: isBuy-%1, price-%2, quantity-%3")
                                                    .arg(ownOrder.isBuy)
                                                    .arg(ownOrder.price)
                                                    .arg(ownOrder.quantity);
                        })
                        .fail([](const std::exception& ex) {
                            LogCCritical(Swaps)
                                << "Swap succeeded: Failed to post order:" << ex.what();
                        });
                }

                return;
            }
        }

        // if we have baseOrder then it was completed
        // if no baseOrder it means that we have completed in one go
        _orderbook->onOwnOrderCompleted(baseOrder.get_value_or(ownOrder));
        _refundableFees->burnRefundableFee({ fee.id() });
    }
}

//==============================================================================

void SwapService::onSwapFailure(const SwapFailure& failure, const storage::RefundableFee& fee)
{
    try {
        auto ownOrder = _orderbook->getOwnOrder(failure.pairId, failure.localId);
        orderbook::OwnLimitOrder limitOrder = ownOrder;
        limitOrder.pairId = failure.pairId;
        // quantity is in base currency, need to fix
        auto amounts = swaps::MakerTakerAmounts::CalculateMakerTakerAmounts(
            limitOrder.quantity, ownOrder.price, ownOrder.isBuy, ownOrder.pairId);
        limitOrder.quantity = amounts.taker.amount;

        auto baseOrder = _orderbook->tryGetBaseOrder(failure.pairId, failure.orderId);

        _orderbook->onOwnOrderSwapFailure(failure.pairId, failure);

        if (failure.role == swaps::SwapRole::Maker) {
            if (baseOrder) {
                placePartialOrder(baseOrder.get(), limitOrder.quantity, fee)
                    .then([ownOrder = limitOrder]() {
                        LogCCInfo(Swaps) << QString("Swap failed: partial order was successfully "
                                                    "reposted: isBuy-%1, price-%2, quantity-%3")
                                                .arg(ownOrder.isBuy)
                                                .arg(ownOrder.price)
                                                .arg(ownOrder.quantity);
                    })
                    .fail([](const std::exception& ex) {
                        LogCCritical(Swaps)
                            << "Swap failed: Failed to repost partial order:" << ex.what();
                    });
            } else {
                placeOrder(limitOrder, fee)
                    .then([ownOrder = limitOrder] {
                        LogCCInfo(Swaps) << QString("Swap failed: order was successfully reposted: "
                                                    "isBuy-%1, price-%2, quantity-%3")
                                                .arg(ownOrder.isBuy)
                                                .arg(ownOrder.price)
                                                .arg(ownOrder.quantity);
                    })
                    .fail([](const std::exception& ex) {
                        LogCCritical(Swaps) << "Swap failed: Failed to repost order:" << ex.what();
                    });
            }
        } else {
            // TODO(yuraolex): get back to this, fee shouldn't be refunded immideately
            //            _refundableFees->refundFee(fee).tapFail([](const std::exception &ex) {
            //                LogCCritical(Swaps) << "Failed to refund orderbook fee on swap
            //                failure, reason:" << ex.what();
            //            });
        }
    } catch (...) {
    }
}

//==============================================================================

void SwapService::init()
{
    qRegisterMetaType<swaps::SwapFailure>("SwapFailure");
    qRegisterMetaType<swaps::SwapSuccess>("SwapSuccess");
    qRegisterMetaType<swaps::SwapDeal>("SwapDeal");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<swaps::SwapService::State>("State");
    qRegisterMetaType<storage::RefundableFee>("storage::RefundableFee");
    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<ResolveRequest>("ResolveRequest");
    qRegisterMetaType<ResolvedTransferResponse>("ResolvedTransferResponse");
    qRegisterMetaType<orderbook::OrderbookApiClient::State>("orderbook::OrderbookApiClient::State");

    LogCCInfo(Swaps) << "Starting swap service, cfg:" << Qt::endl
                     << "version:" << ORDERBOOK_MIN_API_VERSION << Qt::endl
                     << "url:" << _cfg.orderbookUrl << Qt::endl
                     << "secretSet:" << !_cfg.botMakerSecret.isEmpty() << Qt::endl
                     << "client_id:" << (_cfg.clientId.isEmpty() ? "not set" : _cfg.clientId)
                     << Qt::endl
                     << "payFee:" << _cfg.payFee;

    Q_ASSERT(_cfg.swapClientFactory);

    _clientsPool = new SwapClientPool(*_cfg.swapClientFactory, this);

    std::map<QString, QString> orderbookParams{
        { orderbook::OrderbookParamsKeys::HEADER_API_VERSION,
            QString::number(ORDERBOOK_MIN_API_VERSION) }
    };

    if (!_cfg.botMakerSecret.isEmpty()) {
        orderbookParams.emplace(
            orderbook::OrderbookParamsKeys::HEADER_BOT_MAKER_SECRET, _cfg.botMakerSecret);
    }

    if (!_cfg.clientId.isEmpty()) {
        orderbookParams.emplace(orderbook::OrderbookParamsKeys::HEADER_CLIENT_ID, _cfg.clientId);
    }

    auto apiClient
        = std::make_unique<orderbook::OrderbookApiClient>(_cfg.orderbookUrl, orderbookParams);

    _orderbook = new orderbook::OrderbookClient(std::move(apiClient), this);
    connect(
        _orderbook, &orderbook::OrderbookClient::stateChanged, this, &SwapService::onCheckState);

    QDir dataDir(_cfg.dataDirPath);

    auto serviceDb = std::make_shared<Utils::LevelDBSharedDatabase>(
        dataDir.absoluteFilePath("service_repository").toStdString(), 10000);

    _paymentNodeRegistration
        = new orderbook::PaymentNodeRegistrationService(*_clientsPool, _orderbook, this);
    _refundableFees = new RefundableFeeManager(
        *_clientsPool, serviceDb, _orderbook, _paymentNodeRegistration, this);

    _rentingManager = new orderbook::ChannelRentingManager(
        *_clientsPool, *_refundableFees, _orderbook, serviceDb, this);

    _peersPool = &_orderbook->peerPool();
    _repository = new SwapRepository(dataDir.absoluteFilePath("history").toStdString(), this);
    _swapManager = new SwapManager(*_peersPool, *_clientsPool, *_repository, *_orderbook, this);
    connect(_swapManager, &SwapManager::swapPaid, this, [this](SwapSuccess deal) {
        if (deal.role == swaps::SwapRole::Maker) {
            auto fee = _refundableFees->feeByOrderId(deal.orderId);
            onSwapSuccess(deal, fee ? fee.get() : storage::RefundableFee{});
            //            Q_ASSERT_X(fee.has_value(), "SwapService::swapPaid_lambda", "Couldn't find
            //            refundable fee in db");
        }
        swapExecuted(deal);
    });
    connect(_swapManager, &SwapManager::swapFailed, this, [this](SwapDeal swap) {
        SwapFailure failedSwap;
        failedSwap.pairId = swap.pairId;
        failedSwap.orderId = swap.orderId;
        failedSwap.localId = swap.localId;
        failedSwap.quantity = swap.quantity.get();
        failedSwap.failureReason = swap.failureReason.get();
        failedSwap.failureMessage = swap.errorMessage.get_value_or({});
        failedSwap.role = swap.role;

        LogCCritical(Swaps) << "Failed to execute swap" << failedSwap.pairId.c_str()
                            << failedSwap.orderId.c_str() << failedSwap.quantity
                            << static_cast<int>(failedSwap.failureReason)
                            << failedSwap.failureMessage.c_str();

        if (swap.role == swaps::SwapRole::Maker) {
            auto fee = _refundableFees->feeByOrderId(failedSwap.orderId);
            onSwapFailure(failedSwap, fee ? fee.get() : storage::RefundableFee{});
        }

        swapFailed(failedSwap);
    });

    _buyBestPriceModel
        = new orderbook::OrderbookBestPriceModel(_orderbook->ordersModel(), true, this);
    _sellBestPriceModel
        = new orderbook::OrderbookBestPriceModel(_orderbook->ordersModel(), false, this);
}

//==============================================================================

Promise<PlaceOrderResult> SwapService::executePlaceOrder(
    const orderbook::LimitOrder& ownLimitOrder, const storage::RefundableFee& fee)
{
    orderbook::OwnLimitOrder ownOrder(ownLimitOrder, orderbook::Local{ {}, 0 }, fee.paymenthash());
    return _orderbook->placeOrder(ownOrder)
        .tapFail(std::bind(&SwapService::handlePlaceOrderFailure, this, fee))
        .then([this, fee](orderbook::PlaceOrderOutcome outcome) {
            return this->handlePlaceOrderResult(outcome, fee);
        });
}

//==============================================================================

Promise<void> SwapService::executeBaseOrderChecks(const orderbook::LimitOrder& ownLimitOrder)
{
    return _orderbook->isPairActivated(ownLimitOrder.pairId).then([this, ownLimitOrder]() {
        QVector<Promise<void>> promises;
        promises.push_back(QtPromise::map(
            QString::fromStdString(ownLimitOrder.pairId).split('_').toVector(),
            [this](QString currency, ...) {
                return _paymentNodeRegistration->ensurePaymentNodeRegistered(currency.toStdString())
                    .then([] { return true; });
            })
                               .then([](QVector<bool>) {}));

        promises.push_back(this->verifyCanSendRecv(ownLimitOrder));
        return QtPromise::all(promises);
    });
}

//==============================================================================

Promise<storage::RefundableFee> SwapService::executePlaceOrderFeePayment(
    const orderbook::LimitOrder& orderbookOrder)
{
    if (_cfg.payFee && _tradingPairs.size() > 0) {
        auto pairId = orderbookOrder.pairId;
        auto it = std::find_if(std::begin(_tradingPairs), std::end(_tradingPairs),
            [&pairId](const auto& tp) { return tp.pairId == pairId; });

        if (it == std::end(_tradingPairs)) {
            throw FeeFailure(FeeFailure::Reason::RemoteError, "No trading pair found");
        }

        return _refundableFees->payPlaceOrderFee(
            *it, orderbookOrder.quantity, orderbookOrder.isBuy);
    }

    return QtPromise::resolve(storage::RefundableFee{});
}

//==============================================================================

Promise<void> SwapService::verifyCanSendRecv(const orderbook::LimitOrder& ownOrder)
{
    auto orderPrice = ownOrder.price;
    // means we are dealing with market order
    // need to estimate price to check routes
    if (orderPrice <= 0) {
        const auto& model = *(ownOrder.isBuy ? _sellBestPriceModel : _buyBestPriceModel);
        orderPrice = model.bestPriceByPairId(ownOrder.pairId);
        if (orderPrice == 0) {
            return Promise<void>::reject(
                std::runtime_error(strprintf("No available data for marker order price")));
        }
    }

    // in case of buy, quantity is in quote currency, in case of sell, in base currency
    auto baseQuantity = ownOrder.isBuy ? swaps::MakerTakerAmounts::CalculateOrderAmount(
                                             ownOrder.quantity, orderPrice, ownOrder.pairId)
                                       : ownOrder.quantity;

    auto makerTakerAmounts = swaps::MakerTakerAmounts::CalculateMakerTakerAmounts(
        baseQuantity, orderPrice, ownOrder.isBuy, ownOrder.pairId);
    if (makerTakerAmounts.maker.units < 10) {
        return Promise<void>::reject(
            std::runtime_error(strprintf("Maker units amount is less than 10 sats %llu %s",
                makerTakerAmounts.maker.units, makerTakerAmounts.maker.currency)));
    }

    if (makerTakerAmounts.taker.units < 10) {
        return Promise<void>::reject(
            std::runtime_error(strprintf("Taker units amount is less than 10 sats %llu %s",
                makerTakerAmounts.taker.units, makerTakerAmounts.taker.currency)));
    }

    auto currencies = QString::fromStdString(ownOrder.pairId).split('_');
    auto baseCurrency = currencies.at(0).toStdString();
    auto quoteCurrency = currencies.at(1).toStdString();

    auto baseClient = _clientsPool->getClient(baseCurrency);
    auto quoteClient = _clientsPool->getClient(quoteCurrency);

    if (!baseClient) {
        return Promise<void>::reject(
            std::runtime_error(strprintf("Client for currency %s not found", baseCurrency)));
    }
    if (!quoteClient) {
        return Promise<void>::reject(
            std::runtime_error(strprintf("Client for currency %s not found", quoteCurrency)));
    }
    // we will handle only lightning case so far
    // TODO(yuraolex): revisit this so it's in abstract way to avoid cast
    if (baseClient->type() != ClientType::Lnd || quoteClient->type() != ClientType::Lnd) {
        return QtPromise::resolve();
    }

    int64_t sumOwnBuyAmount = 0, sumOwnBuyTotal = 0;
    int64_t sumOwnSellAmount = 0, sumOwnSellTotal = 0;

    _orderbook->ownOrdersByPairId(
        [&](const orderbook::OwnOrder& ownOrder) {
            auto quantity = ownOrder.quantity;

            if (!ownOrder.openPortions.empty()) {
                quantity = std::accumulate(std::begin(ownOrder.openPortions),
                    std::end(ownOrder.openPortions), 0,
                    [](int64_t accum, const auto& portion) { return accum + portion.quantity; });
            }

            auto amounts = swaps::MakerTakerAmounts::CalculateMakerTakerAmounts(
                quantity, ownOrder.price, ownOrder.isBuy, ownOrder.pairId);

            if (ownOrder.isBuy) {
                sumOwnBuyTotal += amounts.taker.amount;
                sumOwnBuyAmount += amounts.maker.amount;
            } else {
                sumOwnSellTotal += amounts.taker.amount;
                sumOwnSellAmount += amounts.maker.amount;
            }
        },
        ownOrder.pairId);

    // Locked balances, for base currency and quote currency
    std::array<std::array<int64_t, 2>, 2> lockedBalance{ std::array<int64_t, 2>{
                                                             sumOwnSellTotal, sumOwnBuyAmount },
        std::array<int64_t, 2>{ sumOwnBuyTotal, sumOwnSellAmount } };

    return QtPromise::all(QVector<Promise<std::vector<LndChannel>>>{
                              static_cast<LndSwapClient*>(baseClient)->activeChannels(),
                              static_cast<LndSwapClient*>(quoteClient)->activeChannels() })
        .then([isBuy = ownOrder.isBuy, makerTakerAmounts, lockedBalance](
                  const QVector<std::vector<LndChannel>>& channels) {
            auto baseBalanses = BuildChannelsBalance(channels.at(0));
            auto quoteBalanses = BuildChannelsBalance(channels.at(1));

            auto verifyCanSend = [&baseBalanses, &quoteBalanses, &lockedBalance](
                                     auto units, bool isBaseCurrency) {
                const auto& balances = isBaseCurrency ? baseBalanses : quoteBalanses;
                const auto& locked = isBaseCurrency ? lockedBalance.at(0) : lockedBalance.at(1);
                return ChannelHasEnoughBalance(balances.at(0) - locked.at(0), units);
            };

            auto verifyCanRecv = [&baseBalanses, &quoteBalanses, &lockedBalance](
                                     auto units, bool isBaseCurrency) {
                const auto& balances = isBaseCurrency ? baseBalanses : quoteBalanses;
                const auto& locked = isBaseCurrency ? lockedBalance.at(0) : lockedBalance.at(1);
                return ChannelHasEnoughBalance(balances.at(1) - locked.at(1), units);
            };

            if (isBuy) {
                if (!verifyCanSend(makerTakerAmounts.taker.amount, false)) {
                    throw std::runtime_error(strprintf("Can't send %llu %s",
                        makerTakerAmounts.taker.amount, makerTakerAmounts.taker.currency));
                }
                if (!verifyCanRecv(makerTakerAmounts.maker.amount, true)) {
                    throw std::runtime_error(strprintf("Can't receive %llu %s",
                        makerTakerAmounts.maker.amount, makerTakerAmounts.maker.currency));
                }
            } else {
                if (!verifyCanSend(makerTakerAmounts.taker.amount, true)) {
                    throw std::runtime_error(strprintf("Can't send %llu %s",
                        makerTakerAmounts.taker.amount, makerTakerAmounts.taker.currency));
                }
                if (!verifyCanRecv(makerTakerAmounts.maker.amount, false)) {
                    throw std::runtime_error(strprintf("Can't receive %llu %s",
                        makerTakerAmounts.maker.amount, makerTakerAmounts.maker.currency));
                }
            }
        });
}

//==============================================================================

Promise<SwapSuccess> SwapService::executeSwapHelper(
    orderbook::PeerOrder maker, orderbook::OwnOrder taker)
{
    OwnOrder ownOrder;
    ownOrder.id = taker.id;
    ownOrder.pairId = taker.pairId;
    ownOrder.price = taker.price;
    ownOrder.quantity = taker.quantity;
    ownOrder.isBuy = taker.isBuy;
    ownOrder.localId = taker.localId;
    ownOrder.type = taker.orderType == orderbook::OwnOrderType::Limit ? swaps::OrderType::Limit
                                                                      : swaps::OrderType::Market;

    PeerOrder peerOrder;
    peerOrder.id = maker.id;
    peerOrder.pairId = maker.pairId;
    peerOrder.price = maker.price;
    peerOrder.quantity = maker.quantity;
    peerOrder.isBuy = !taker.isBuy;
    peerOrder.peerPubKey = maker.peerPubKey;

    LogCDebug(Swaps) << "onOrderMatched OWNORDER" << ownOrder.id.c_str() << ownOrder.price
                     << ownOrder.quantity << ownOrder.isBuy << ownOrder.pairId.c_str();

    LogCDebug(Swaps) << "onOrderMatched PEER" << peerOrder.id.c_str() << peerOrder.price
                     << peerOrder.quantity << peerOrder.isBuy << peerOrder.pairId.c_str();

    return _swapManager->executeSwap(peerOrder, ownOrder);
}

//==============================================================================

Promise<PlaceOrderResult> SwapService::placePartialOrder(
    const orderbook::OwnOrder& baseOrder, int64_t orderPortion, const storage::RefundableFee& fee)
{
    return _orderbook
        ->placeOrderPortion(baseOrder.pairId, baseOrder.localId, orderPortion, baseOrder.orderType)
        .tapFail(std::bind(&SwapService::handlePlaceOrderFailure, this, fee))
        .then([this, fee](orderbook::PlaceOrderOutcome outcome) {
            return handlePlaceOrderResult(outcome, fee);
        });
}

//==============================================================================

Promise<PlaceOrderResult> SwapService::placeOrder(
    const orderbook::OwnLimitOrder& order, const storage::RefundableFee& fee)
{
    return executeBaseOrderChecks(order).then([this, order, fee] {
        return _orderbook->placeOrder(order)
            .tapFail(std::bind(&SwapService::handlePlaceOrderFailure, this, fee))
            .then([this, fee](orderbook::PlaceOrderOutcome outcome) {
                return this->handlePlaceOrderResult(outcome, fee);
            });
    });
}

//==============================================================================

Promise<PlaceOrderResult> SwapService::handlePlaceOrderResult(
    orderbook::PlaceOrderOutcome outcome, storage::RefundableFee fee)
{
    if (auto ownOrder = boost::get<orderbook::OwnOrder>(&outcome)) {
        _refundableFees->trackPlacedOrderFee(fee.id(), ownOrder->id);
        return QtPromise::resolve(PlaceOrderResult(*ownOrder));
    } else if (auto trade = boost::get<orderbook::Trade>(&outcome)) {
        return this->executeSwapHelper(trade->peerOrder, trade->ownOrder)
            .tap([fee, this](SwapSuccess swapSuccess) { this->onSwapSuccess(swapSuccess, fee); })
            .tapFail([fee, this](SwapFailure failure) { this->onSwapFailure(failure, fee); })
            .then([](SwapSuccess swapSuccess) { return PlaceOrderResult(swapSuccess); });
    } else if (auto orderPort = boost::get<orderbook::OrderPortion>(&outcome)) {
        auto baseOrder = _orderbook->tryGetBaseOrder(orderPort->pairId, orderPort->orderId);
        if (baseOrder != boost::none) {
            return QtPromise::resolve(PlaceOrderResult(baseOrder.get()));
        }
    }

    return Promise<PlaceOrderResult>::reject(std::runtime_error("Invalid place order response"));
}

//==============================================================================

void SwapService::handlePlaceOrderFailure(const storage::RefundableFee& fee)
{
    // TODO(yuraolex): get back to this, fee shouldn't be refunded immideately
    //    _refundableFees->refundFee(fee).fail([](const std::exception &ex) {
    //        LogCCritical(Orderbook) << "Failed to refund fee after failed placeOrder" <<
    //        ex.what();
    //    });
}

//==============================================================================

FeeFailure::FeeFailure(FeeFailure::Reason reason, std::string message)
    : message(message)
    , reason(reason)
{
}

//==============================================================================

QUrl SwapService::Config::DefaultOrderbookUrl()
{
    return QUrl("wss://orderbook.stakenet.io/api/ws");
}

//==============================================================================

QUrl SwapService::Config::StagingOrderbookUrl()
{
    return QUrl("wss://stg-orderbook.stakenet.io/api/ws");
}

//==============================================================================
}
