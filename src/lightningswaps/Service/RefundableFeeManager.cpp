#include "RefundableFeeManager.hpp"
#include <EthCore/Types.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/OrderbookRefundableFees.hpp>
#include <Service/PaymentNodeRegistrationService.hpp>
#include <Service/RefundableFeeManagerState.hpp>
#include <Swaps/AbstractSwapClientPool.hpp>
#include <Swaps/AbstractSwapConnextClient.hpp>
#include <Swaps/AbstractSwapLndClient.hpp>
#include <Swaps/Types.hpp>
#include <Utils/GenericProtoDatabase.hpp>
#include <Utils/Logging.hpp>
#include <utilstrencodings.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>
#include <tinyformat.h>

using namespace boost::adaptors;

namespace swaps {

//==============================================================================

static boost::optional<uint64_t> FindRefundableFeeByPaymentHash(
    const std::unordered_map<uint64_t, storage::RefundableFee>& where,
    const std::string& paymentHash)
{
    auto it = std::find_if(std::begin(where), std::end(where),
        [&paymentHash](const auto& it) { return it.second.paymenthash() == paymentHash; });

    return it != std::end(where) ? boost::make_optional(it->first) : boost::none;
}

//==============================================================================

RefundableFeeManager::RefundableFeeManager(const AbstractSwapClientPool& clients,
    std::shared_ptr<Utils::LevelDBSharedDatabase> sharedDb,
    QPointer<orderbook::OrderbookClient> client,
    QPointer<orderbook::PaymentNodeRegistrationService> registrationService, QObject* parent)
    : QObject(parent)
    , _swapClients(clients)
    , _refundableFeeDb(std::make_unique<Utils::GenericProtoDatabase<storage::RefundableFee>>(
          sharedDb, "refundable_order_fee"))
    , _orderbook(client)
    , _state(new RefundableFeeManagerState(*_refundableFeeDb, this))
    , _paymentNodeRegistration(registrationService)
{
    connect(_state, &RefundableFeeManagerState::refundableAmountChanged, this,
        &RefundableFeeManager::onRefundableAmountChanged);
    _refundableFeeDb->load();

    auto onOrderbookStateChanged = [this](orderbook::OrderbookApiClient::State state) {
        if (state == orderbook::OrderbookApiClient::State::Connected) {
            _state->refresh();
        }
    };

    connect(_orderbook, &orderbook::OrderbookClient::stateChanged, this, onOrderbookStateChanged);
    onOrderbookStateChanged(_orderbook->state());
}

//==============================================================================

RefundableFeeManager::~RefundableFeeManager() {}

//==============================================================================

Promise<storage::RefundableFee> RefundableFeeManager::payPlaceOrderFee(
    const orderbook::TradingPair& tradingPair, int64_t quantity, bool isBuy)
{
    auto currency
        = QString::fromStdString(tradingPair.pairId).split('_').at(isBuy ? 1 : 0).toStdString();
    if (swaps::CLIENT_TYPE_PER_CURRENCY.at(currency) == swaps::ClientType::Lnd) {
        return payLndPlaceOrderFee(tradingPair, quantity, isBuy);
    } else {
        return payConnextPlaceOrderFee(tradingPair, quantity, isBuy);
    }
}

//==============================================================================

Promise<storage::RefundableFee> RefundableFeeManager::payLndPlaceOrderFee(
    const orderbook::TradingPair& tradingPair, int64_t quantity, bool isBuy)
{
    auto currency
        = QString::fromStdString(tradingPair.pairId).split('_').at(isBuy ? 1 : 0).toStdString();
    auto fee = CalculatePlaceOrderFee(tradingPair, quantity, isBuy);
    return _orderbook->feeRefunding()
        ->getLndFeePaymentRequest(currency, fee)
        .fail([] {
            return Promise<std::pair<std::string, std::string>>::reject(
                (FeeFailure(FeeFailure::Reason::RemoteError, "Failed to get fee payment request")));
        })
        .then([currency, fee, pairId = tradingPair.pairId, isBuy, this](
                  std::pair<std::string, std::string> paymentRequest) {
            // if no payment request means that orderbook doesn't require fee, so we don't pay.
            if (paymentRequest.second.empty()) {
                return QtPromise::resolve(storage::RefundableFee{});
            } else {
                return this->executeLndPlaceOrderFeePayment(currency, paymentRequest.second)
                    .then([=](std::string paymentHash) {
                        storage::RefundableFee feeDbEntry;
                        feeDbEntry.set_state(
                            storage::RefundableFee_State::RefundableFee_State_PAID);
                        feeDbEntry.set_initialamount(fee);
                        feeDbEntry.set_currency(currency);
                        feeDbEntry.set_paymenthash(paymentHash);
                        feeDbEntry.set_timestamp(QDateTime::currentSecsSinceEpoch());
                        feeDbEntry.set_is_buy(isBuy);
                        feeDbEntry.set_pairid(pairId);
                        feeDbEntry.set_type(
                            storage::RefundableFee_Type::RefundableFee_Type_ORDER_FEE);
                        _refundableFeeDb->save(feeDbEntry);
                        return feeDbEntry;
                    })
                    .tap([this](storage::RefundableFee fee) { this->refundableFeeAdded(fee); });
            }
        });
}

//==============================================================================

Promise<storage::RefundableFee> RefundableFeeManager::payConnextPlaceOrderFee(
    const orderbook::TradingPair& tradingPair, int64_t quantity, bool isBuy)
{
    auto currency
        = QString::fromStdString(tradingPair.pairId).split('_').at(isBuy ? 1 : 0).toStdString();
    auto fee = CalculatePlaceOrderFee(tradingPair, quantity, isBuy);
    return _orderbook->feeRefunding()
        ->getConnextFeePaymentHash(currency)
        .fail([] {
            return Promise<orderbook::OrderbookRefundableFees::ConnextPaymentInfoResponse>::reject(
                (FeeFailure(FeeFailure::Reason::RemoteError, "Failed to get fee payment request")));
        })
        .then([currency, fee, pairId = tradingPair.pairId, isBuy, this](
                  orderbook::OrderbookRefundableFees::ConnextPaymentInfoResponse
                      paymentInfoResponse) {
            return this
                ->executeConnextPlaceOrderFeePayment(currency, fee,
                    paymentInfoResponse.publicidentifier(), paymentInfoResponse.paymenthash())
                .then([=](std::string paymentHash) {
                    storage::RefundableFee feeDbEntry;
                    feeDbEntry.set_state(storage::RefundableFee_State::RefundableFee_State_PAID);
                    feeDbEntry.set_initialamount(fee);
                    feeDbEntry.set_currency(currency);
                    feeDbEntry.set_paymenthash(paymentHash.erase(0, 2));
                    feeDbEntry.set_timestamp(QDateTime::currentSecsSinceEpoch());
                    feeDbEntry.set_is_buy(isBuy);
                    feeDbEntry.set_pairid(pairId);
                    feeDbEntry.set_type(storage::RefundableFee_Type::RefundableFee_Type_ORDER_FEE);
                    _refundableFeeDb->save(feeDbEntry);
                    this->refundableFeeAdded(feeDbEntry);
                    return feeDbEntry;
                })
                .tap([this](storage::RefundableFee fee) { this->refundableFeeAdded(fee); });
        });
}

//==============================================================================

Promise<RefundableFeeManager::RentingPaymentFeeResponse> RefundableFeeManager::payChannelRentingFee(
    std::string requestedCurrency, std::string payingCurrency, std::string paymentRequest,
    int64_t initialAmount)
{
    return this
        ->executeRentingPayment(requestedCurrency, payingCurrency, paymentRequest, initialAmount)
        .then([this, initialAmount, requestedCurrency, payingCurrency](
                  RentingPaymentFeeResponse paymentInfo) {
            storage::RefundableFee& fee = paymentInfo.fee;
            fee.set_state(storage::RefundableFee_State::RefundableFee_State_PAID);
            fee.set_initialamount(initialAmount);
            fee.set_currency(payingCurrency);
            fee.set_paymenthash(paymentInfo.paymentHash);
            fee.set_timestamp(QDateTime::currentSecsSinceEpoch());
            fee.set_pairid(requestedCurrency + "_" + payingCurrency);
            fee.set_type(storage::RefundableFee_Type::RefundableFee_Type_CHANNEL_RENTAL_FEE);
            _refundableFeeDb->save(fee);
            return paymentInfo;
        })
        .tapFail([](const std::exception& ex) {
            LogCCritical(Orderbook) << "Failed to execute renting payment: " << ex.what();
            throw std::runtime_error("renting payment failed");
        });
}

//==============================================================================

Promise<RefundableFeeManager::ExtendPaymentFeeResponse>
RefundableFeeManager::payExtendTimeRentingFee(
    std::string payingCurrency, std::string paymentRequest)
{
    return this->executeExtendRentedChannelPayment(payingCurrency, paymentRequest)
        .then([this, payingCurrency](ExtendPaymentResponse response) {
            storage::RefundableFee fee;
            fee.set_state(storage::RefundableFee_State::RefundableFee_State_PAID);
            fee.set_initialamount(std::get<1>(response));
            fee.set_currency(payingCurrency);
            fee.set_paymenthash(std::get<0>(response));
            fee.set_timestamp(QDateTime::currentSecsSinceEpoch());
            fee.set_type(storage::RefundableFee_Type::RefundableFee_Type_CHANNEL_EXTEND_FEE);
            _refundableFeeDb->save(fee);
            return ExtendPaymentFeeResponse{ std::get<0>(response), fee };
        })
        .tapFail([](const std::exception& ex) {
            LogCCritical(Orderbook)
                << "Failed to execute extend renting channel payment: " << ex.what();
            throw std::runtime_error("extend rented channel payment failed");
        });
}

//==============================================================================

void RefundableFeeManager::trackPlacedOrderFee(uint64_t id, std::string orderId)
{
    if (id > 0) {
        _placedOrders.emplace(orderId, id);
    }
}

//==============================================================================

void RefundableFeeManager::burnRefundableFee(const std::vector<uint64_t>& ids)
{
    _refundableFeeDb->erase(ids);
    for (auto&& id : ids) {
        refundableFeeBurnt(id);
        for (auto it = _placedOrders.begin(); it != _placedOrders.end();) {
            if (it->second == id) {
                it = _placedOrders.erase(it);
            } else {
                ++it;
            }
        }
    }
}

//==============================================================================

boost::optional<storage::RefundableFee> RefundableFeeManager::feeByOrderId(
    const std::string& orderId)
{
    if (_placedOrders.count(orderId) > 0) {
        return _refundableFeeDb->get(_placedOrders.at(orderId));
    }

    return boost::none;
}

//==============================================================================

RefundableFeeManagerState* RefundableFeeManager::state() const
{
    return _state;
}

//==============================================================================

int64_t RefundableFeeManager::CalculatePlaceOrderFee(
    const orderbook::TradingPair& tradingPair, int64_t quantity, bool isBuy)
{
    return std::max<int64_t>(
        static_cast<int64_t>(
            qRound64(quantity * (isBuy ? tradingPair.buyFeePercent : tradingPair.sellFeePercent))),
        1);
}

//==============================================================================

void RefundableFeeManager::onRefundableAmountChanged(
    std::string currency, const RefundableFeeManagerState::RefundableAmount& amount)
{
    if (_orderbook->state() != orderbook::OrderbookApiClient::State::Connected) {
        return;
    }

    if (amount.available > 0) {

        if (!_swapClients.hasClient(currency)) {
            return;
        }

        std::vector<orderbook::OrderbookRefundableFees::RefundableFeePayment> data;

        auto filter = [this, currency](const auto& fee) {
            auto dataFilter = fee.second.currency() == currency
                && RefundableFeeManagerState::IsRefundable(fee.second);

            if (dataFilter) {
                auto it = std::find_if(_placedOrders.begin(), _placedOrders.end(),
                    [feeId = fee.second.id()](const auto& item) { return item.second == feeId; });

                return it == _placedOrders.end();
            }

            return dataFilter;
        };

        auto transform = [](const auto& it) {
            const storage::RefundableFee& fee = it.second;
            auto payHash
                = QByteArray::fromHex(QByteArray::fromStdString(fee.paymenthash())).toStdString();

            return std::make_tuple(
                std::vector<unsigned char>(payHash.begin(), payHash.end()), fee.initialamount());
        };

        auto transformIds = [](const auto& it) { return it.second.id(); };

        auto rng = _refundableFeeDb->values() | filtered(filter);
        std::vector<uint64_t> ids;
        boost::push_back(data, rng | transformed(transform));
        boost::push_back(ids, rng | transformed(transformIds));

        if (data.empty()) {
            return;
        }

        auto refunding = _orderbook->feeRefunding();
        refunding->refundableAmount(currency, data)
            .then([currency, ids, data, this](int64_t amount) {
                if (amount <= 0) {
                    return Promise<bool>::resolve(false);
                }

                auto client = _swapClients.getClient(currency);
                if (!client) {
                    return Promise<bool>::reject(std::runtime_error(
                        strprintf("Failed to find lnd client for currency: %s", currency)));
                }
                if (swaps::CLIENT_TYPE_PER_CURRENCY.at(currency) == swaps::ClientType::Lnd) {
                    return _orderbook->feeRefunding()->refundFee(currency, data).then([ids, this] {
                        this->burnRefundableFee(ids);
                        return true;
                    });
                }
                return Promise<bool>::resolve(true);
            })
            .tapFail([this, currency](const std::exception& ex) {
                LogCCritical(Orderbook) << "Fee refunding failed, reason:" << ex.what();
                this->onHandleRefundError(QString::fromStdString(ex.what()), currency);
            })
            .then([this, currency](bool refresh) {
                if (refresh) {
                    _state->refresh(currency);
                }
            });
    }
}

//==============================================================================

Promise<std::string> RefundableFeeManager::executeLndPlaceOrderFeePayment(
    std::string currency, std::string paymentRequest)
{
    return QtPromise::attempt([=] {
        if (auto client = _swapClients.getClient(currency)) {
            return ensurePaymentNodeRegistered(currency).then([client, paymentRequest] {
                auto lndClient = qobject_cast<AbstractSwapLndClient*>(client);
                return lndClient
                    ->sendPayment(
                        paymentRequest, lndtypes::LightningPaymentReason::PAYMENT_ORDERBOOK_FEE)
                    .then([](lnrpc::Payment payment) { return payment.payment_hash(); });
            });
        }

        return Promise<std::string>::reject(
            FeeFailure(FeeFailure::Reason::NoRouteFound, "No route found"));
    });
}

//==============================================================================

Promise<std::string> RefundableFeeManager::executeConnextPlaceOrderFeePayment(
    std::string currency, int64_t fee, std::string publicIdentifier, std::string paymentHash)
{
    return QtPromise::attempt([=] {
        if (auto client = _swapClients.getClient(currency)) {
            return ensurePaymentNodeRegistered(currency).then(
                [client, fee, publicIdentifier, paymentHash, currency] {
                    auto connextClient = qobject_cast<AbstractSwapConnextClient*>(client);

                    QVariantMap payload;

                    payload["amount"] = QString::fromStdString(eth::ConvertDenominations(
                        eth::u256{ fee }, 8, UNITS_PER_CURRENCY.at(currency))
                                                                   .str());
                    payload["recipient"] = QString::fromStdString(publicIdentifier);

                    QVariantMap details;
                    auto lockHash = QString("0x%1").arg(QString::fromStdString(
                        QByteArray::fromStdString(paymentHash).toHex().toStdString()));
                    details["lockHash"] = lockHash;
                    details["expiry"] = 2000000000;

                    payload["details"] = details;

                    return connextClient->sendPaymentWithPayload(payload).then(
                        [](std::string response) { return response; });
                });
        }

        return Promise<std::string>::reject(
            FeeFailure(FeeFailure::Reason::NoRouteFound, "No route found"));
    });
}

//==============================================================================

Promise<RefundableFeeManager::RentingPaymentFeeResponse>
RefundableFeeManager::executeRentingPayment(std::string requestedCurrency,
    std::string payingCurrency, std::string paymentRequest, int64_t initialAmount)
{
    return QtPromise::attempt([=] {
        ensurePaymentNodeRegistered(requestedCurrency);

        if (swaps::CLIENT_TYPE_PER_CURRENCY.at(payingCurrency) == swaps::ClientType::Lnd) {

            auto lndClient
                = qobject_cast<AbstractSwapLndClient*>(_swapClients.getClient(payingCurrency));
            return lndClient
                ->sendPayment(paymentRequest, lndtypes::LightningPaymentReason::PAYMENT_RENTAL)
                .then([requestedCurrency, payingCurrency /*, pubkey*/](lnrpc::Payment payment) {
                    return RentingPaymentFeeResponse{ /*pubkey,*/ payment.payment_hash(),
                        storage::RefundableFee{} };
                });

        } else {
            auto connextClient
                = qobject_cast<AbstractSwapConnextClient*>(_swapClients.getClient(payingCurrency));
            QVariantMap payload;

            payload["amount"] = QString::fromStdString(eth::ConvertDenominations(
                eth::u256{ initialAmount }, 8, UNITS_PER_CURRENCY.at(payingCurrency))
                                                           .str());
            payload["recipient"]
                = QString("vector6At5HhbfhcE1p1SBTxAL5ej71AGWkSSxrSTmdj6wuFHquJ1hg8");

            QVariantMap details;
            auto lockHash = QString("0x%1").arg(QString::fromStdString(
                QByteArray::fromStdString(paymentRequest).toHex().toStdString()));
            details["lockHash"] = lockHash;
            details["expiry"] = 2000000000;

            payload["details"] = details;

            return connextClient->sendPaymentWithPayload(payload).then(
                [requestedCurrency, payingCurrency /*, pubkey*/](std::string response) {
                    return RentingPaymentFeeResponse{ /*pubkey,*/ response,
                        storage::RefundableFee{} };
                });
            ;
        }
    });
}

//==============================================================================

Promise<RefundableFeeManager::ExtendPaymentResponse>
RefundableFeeManager::executeExtendRentedChannelPayment(
    std::string payingCurrency, std::string paymentRequest)
{
    return QtPromise::attempt([=] {
        //        if (swaps::CLIENT_TYPE_PER_CURRENCY.at(payingCurrency) == swaps::ClientType::Lnd)
        //        {
        auto lndClient
            = qobject_cast<AbstractSwapLndClient*>(_swapClients.getClient(payingCurrency));
        return ensurePaymentNodeRegistered(payingCurrency).then([lndClient, paymentRequest] {
            return lndClient->decodePayRequest(paymentRequest)
                .then([lndClient, paymentRequest](LightningPayRequest payRequest) {
                    return lndClient
                        ->sendPayment(paymentRequest,
                            lndtypes::LightningPaymentReason::PAYMENT_RENTAL_EXTENSION)
                        .then([amount = payRequest.numSatoshis](lnrpc::Payment payment) {
                            return ExtendPaymentResponse{ payment.payment_hash(), amount };
                        });
                });
        });
        //        } else {
        //        }
        // TODO: add amount and send payment

        //            auto connextClient
        //                =
        //                qobject_cast<AbstractSwapConnextClient*>(_swapClients.getClient(payingCurrency));
        //            QVariantMap payload;

        //            payload["amount"] = QString::fromStdString(
        //                eth::ConvertDenominations(eth::u256{ initialAmount }, 8,
        //                UNITS_PER_CURRENCY.at(payingCurrency))
        //                    .str());
        //            payload["recipient"] =
        //            QString("vector6At5HhbfhcE1p1SBTxAL5ej71AGWkSSxrSTmdj6wuFHquJ1hg8");

        //            QVariantMap details;
        //            auto lockHash = QString("0x%1").arg(QString::fromStdString(
        //                QByteArray::fromStdString(paymentRequest).toHex().toStdString()));
        //            details["lockHash"] = lockHash;
        //            details["expiry"] = 2000000000;

        //            payload["details"] = details;

        //            return connextClient->sendPaymentWithPayload(payload).then([requestedCurrency,
        //            payingCurrency /*, pubkey*/](std::string response) {
        //                return RentingPaymentFeeResponse{ /*pubkey,*/ response,
        //                    storage::RefundableFee{} };
        //            });;
        //        }
    });
}

//==============================================================================

void RefundableFeeManager::onHandleRefundError(QString errorMsg, std::string currency)
{
    std::vector<uint64_t> ids;
    if (errorMsg.startsWith("Refund for")) {
        // Refund for E639DA064347C40B62466E9635ACF59BE0A0F89839C1065C6A8A079EABBED9D9 in XSN
        // already exists.
        auto split = errorMsg.split(QChar(' '));
        if (split.size() > 2) {
            auto paymentHash = split.at(2).toLatin1();
            if (auto opt = FindRefundableFeeByPaymentHash(
                    _refundableFeeDb->values(), paymentHash.toLower().toStdString())) {
                ids.emplace_back(*opt);
            }
        }
    } else if (errorMsg.startsWith("There are invalid payment hash")) {
        auto arrayStr = errorMsg.mid(errorMsg.indexOf('[') + 1).chopped(1);
        auto values = arrayStr.split(QString(", "));
        for (auto paymentHash : values) {
            if (auto opt = FindRefundableFeeByPaymentHash(
                    _refundableFeeDb->values(), paymentHash.toLower().toStdString())) {
                ids.emplace_back(*opt);
            }
        }

    } else if (errorMsg.mid(errorMsg.indexOf('[') + 1).startsWith("Fee with payment hash")) {
        // [Fee with payment hash
        //  5FE5CDE69E958D8DE556A7F31661D20357FE71FCF6B760E9F3EC113F7F7A7012 is locked for
        //  order df0b6ee2-4031-4f50-a998-84d39a12f8c6, Fee with payment hash
        //  DAB0AB81FED11400A238B3AC9B4A42B56C537C6AF5226EE06186E6F7B38128BB is locked for
        //  order 7c4ca788-5a97-48f2-aa79-85b777d6628f]

        auto arrayStr = errorMsg.mid(errorMsg.indexOf('[') + 1).chopped(1);

        for (auto error : arrayStr.split(QString(", "))) {
            auto splitMsgArray = error.split(QString(" "));
            if (splitMsgArray.size() > 4) {
                auto paymentHash = splitMsgArray[4];

                if (auto opt = FindRefundableFeeByPaymentHash(
                        _refundableFeeDb->values(), paymentHash.toLower().toStdString())) {
                    ids.emplace_back(*opt);
                }
            }
        }
    }

    if (!ids.empty()) {
        _refundableFeeDb->erase(ids);
        _state->refresh(currency);
    }
}

//==============================================================================

Promise<void> RefundableFeeManager::ensurePaymentNodeRegistered(std::string currency)
{
    return _paymentNodeRegistration->ensurePaymentNodeRegistered(currency);
}

//==============================================================================
}
