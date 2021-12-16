#include "OrderbookClient.hpp"
#include <Orderbook/OrderbookApiClient.hpp>
#include <Orderbook/OrderbookChannelRenting.hpp>
#include <Orderbook/OrderbookEventDispatcher.hpp>
#include <Orderbook/OrderbookRefundableFees.hpp>
#include <Orderbook/OrderbookSwapPeerPool.hpp>
#include <Orderbook/TradingOrdersModel.hpp>
#include <Utils/Logging.hpp>
#include <cmath>
#include <tinyformat.h>

#include <QUuid>

namespace orderbook {

using namespace io;

//==============================================================================

template <class T> static PeerOrder ConvertToPeerOrder(const T& match)
{
    const auto& order = match.ordermatchedwith();
    const auto price = ConvertBigInt(order.details().price());
    auto quantity = ConvertBigInt(order.details().funds());

    const bool isBuy = order.side() == io::stakenet::orderbook::protos::Order_OrderSide_buy;
    if (isBuy) {
        quantity
            = swaps::MakerTakerAmounts::CalculateOrderAmount(quantity, price, order.tradingpair());
    }

    return PeerOrder(LimitOrder{ MarketOrder{ quantity, order.tradingpair(), isBuy }, price },
        Stamp{ OrderIdentifier{ order.details().orderid() }, match.trade().executedon(), quantity },
        Remote{ match.trade().existingorderid() });
}

//==============================================================================

static Command ConvertToPlaceOrderCmd(OwnLimitOrder ownOrder)
{
    Command msgCommand;
    auto cmd = new stakenet::orderbook::protos::PlaceOrderCommand;
    auto ord = new stakenet::orderbook::protos::Order;
    auto ordDetails = new stakenet::orderbook::protos::OrderDetails;

    ord->set_tradingpair(ownOrder.pairId);
    ord->set_allocated_details(ordDetails);
    ord->set_side([role = ownOrder.isBuy] {
        return role ? stakenet::orderbook::protos::Order_OrderSide::Order_OrderSide_buy
                    : stakenet::orderbook::protos::Order_OrderSide::Order_OrderSide_sell;
    }());

    // funds are always in needed currency
    ordDetails->set_allocated_funds(ConvertToBigInt(ownOrder.quantity).release());

    // means limit order
    if (ownOrder.price > 0) {
        ordDetails->set_allocated_price(ConvertToBigInt(ownOrder.price).release());
        ord->set_type(stakenet::orderbook::protos::Order_OrderType::Order_OrderType_limit);
    } else {
        ord->set_type(stakenet::orderbook::protos::Order_OrderType::Order_OrderType_market);
    }

    cmd->set_allocated_order(ord);
    cmd->set_paymenthash(
        QByteArray::fromHex(QByteArray::fromStdString(ownOrder.feePaymentHash)).toStdString());
    msgCommand.set_allocated_placeorder(cmd);

    return msgCommand;
}

//==============================================================================

OrderbookClient::OrderbookClient(std::unique_ptr<OrderbookApiClient> client, QObject* parent)
    : AbstractOrderbookClient(parent)
    , _apiClient(client.release())
    , _dispatcher(new OrderbookEventDispatcher(*_apiClient, this))
    , _peerPool(new OrderbookSwapPeerPool(*this, this))
{
    init();
}

//==============================================================================

OrderbookClient::~OrderbookClient() {}

//==============================================================================

boost::optional<OwnOrder> OrderbookClient::tryGetOwnOrder(
    std::string pairId, std::string orderId) const
{
    if (_myorders.count(pairId) > 0) {
        const auto& orders = _myorders.at(pairId);
        auto it = findOrderById(orders, pairId, orderId);

        if (it != std::end(orders)) {
            return it->second;
        }
    }

    return boost::none;
}

//==============================================================================

OwnOrder orderbook::OrderbookClient::getOwnOrder(std::string pairId, std::string orderId) const
{
    if (auto portionOpt = tryGetOrderPortion(pairId, orderId)) {
        return portionOpt.get();
    } else if (auto orderOpt = tryGetOwnOrder(pairId, orderId)) {
        return orderOpt.get();
    }

    LogCDebug(Swaps) << "Fetching:" << pairId.data() << orderId.data();
    LogCDebug(Swaps) << "Own orders:";
    for (const auto& [key, value] : _myorders) {
        LogCDebug(Swaps) << "Pair:" << key.data();
        for (const auto& [id, order] : value) {
            LogCDebug(Swaps) << id.data() << order.id.data() << order.localId.data();
        }
    }
    LogCDebug(Swaps) << "MyPartialOrdersIndex:";
    for (const auto& [key, value] : _partialOrdersIndex) {
        LogCDebug(Swaps) << "Pair:" << key.data();
        for (const auto& [partial, base] : value) {
            LogCDebug(Swaps) << partial.data() << base.data();
        }
    }
    LogCDebug(Swaps) << "LocalIndex:";
    for (const auto& [key, value] : _orderIdToLocalIdIndex) {
        LogCDebug(Swaps) << "Pair:" << key.data();
        for (const auto& [partial, base] : value) {
            LogCDebug(Swaps) << partial.data() << base.data();
        }
    }

    throw std::runtime_error("Own order with id " + orderId + " not found");
}

//==============================================================================

boost::optional<OrderbookClient::MatchedTrade> OrderbookClient::tryGetMatchedTrade(
    std::string pairId, std::string orderId)
{
    if (_matchedOrders.count(pairId) > 0) {
        if (_matchedOrders.at(pairId).count(orderId) > 0) {
            return _matchedOrders.at(pairId).at(orderId);
        }
    }

    return boost::none;
}

//==============================================================================

boost::optional<OwnOrder> OrderbookClient::tryGetOrderPortion(
    std::string pairId, std::string partialOrderId) const
{
    if (_partialOrdersIndex.count(pairId) > 0) {
        if (_partialOrdersIndex.at(pairId).count(partialOrderId) > 0) {
            if (auto ownBaseOrder
                = tryGetOwnOrder(pairId, _partialOrdersIndex.at(pairId).at(partialOrderId))) {
                auto it = std::find_if(std::begin(ownBaseOrder->openPortions),
                    std::end(ownBaseOrder->openPortions), [partialOrderId](const auto& portion) {
                        return portion.orderId == partialOrderId;
                    });

                if (it != std::end(ownBaseOrder->openPortions)) {
                    OwnOrder partialOrder(*ownBaseOrder);
                    partialOrder.openPortions.clear();
                    partialOrder.id = it->orderId;
                    partialOrder.quantity = it->quantity;
                    return boost::make_optional(partialOrder);
                }
            }
        }
    }

    return boost::none;
}

//==============================================================================

boost::optional<OwnOrder> OrderbookClient::tryGetBaseOrder(
    std::string pairId, std::string partialOrderId) const
{
    if (_partialOrdersIndex.count(pairId) > 0) {
        if (_partialOrdersIndex.at(pairId).count(partialOrderId) > 0) {
            return tryGetOwnOrder(pairId, _partialOrdersIndex.at(pairId).at(partialOrderId));
        }
    }

    return boost::none;
}

//==============================================================================

void OrderbookClient::start()
{
    _apiClient->open();
}

//==============================================================================

Promise<void> OrderbookClient::registerLndKey(std::string currency, std::string pubkey)
{
    Command msgCommand;
    auto cmd = msgCommand.mutable_registerpublickeycommand();
    cmd->set_currency(std::move(currency));
    cmd->set_publickey(std::move(pubkey));
    return _apiClient->makeRequest(msgCommand).then([](OrderbookResponse) {});
}

//==============================================================================

Promise<void> OrderbookClient::registerConnextKey(std::string currency, std::string pubkey)
{
    Command msgCommand;
    auto cmd = msgCommand.mutable_registerpublicidentifiercommand();
    cmd->set_currency(std::move(currency));
    cmd->set_publicidentifier(std::move(pubkey));
    LogCDebug(Orderbook) << "Sending orderbook request:" << msgCommand.DebugString().data();
    return _apiClient->makeRequest(msgCommand).then([](OrderbookResponse) {});
}

//==============================================================================

Promise<void> OrderbookClient::subscribe(std::string pairId)
{
    if (_subscriptions.count(pairId) == 0) {
        _subscriptions.emplace(pairId);
        return _tradingOrdersModel->subscribe(pairId);
    } else {
        return QtPromise::resolve();
    }
}

//==============================================================================

Promise<void> OrderbookClient::unsubscribe(std::string pairId)
{
    if (_subscriptions.count(pairId) > 0) {
        _subscriptions.erase(pairId);
        return _tradingOrdersModel->unsubsribe(pairId);
    } else {
        return QtPromise::resolve();
    }
}

//==============================================================================

std::vector<std::string> OrderbookClient::activatedPairs() const
{
    std::vector<std::string> activatedPairs;
    for (auto&& pair : _subscriptions) {
        activatedPairs.emplace_back(pair);
    }

    return activatedPairs;
}

//==============================================================================

Promise<void> OrderbookClient::isPairActivated(std::string pairId)
{
    if (_subscriptions.count(pairId) > 0) {
        return QtPromise::resolve();
    } else {
        throw std::runtime_error("Trading pair is not activated");
    }
}

//==============================================================================

Promise<OrderbookClient::TradingPairInfo> OrderbookClient::tradingPairs()
{
    Command getTraidingPairsCmd;
    getTraidingPairsCmd.set_allocated_gettradingpairs(
        new io::stakenet::orderbook::protos::GetTradingPairsCommand);
    return _apiClient->makeRequest(getTraidingPairsCmd).then([](OrderbookResponse response) {
        auto pairs = response.gettradingpairsresponse();
        std::vector<TradingPair> result;
        for (auto&& entry : pairs.tradingpairs()) {
            LogCDebug(Orderbook) << "Traiding pair:" << entry.id().c_str() << "buyInterval:"
                                 << QString("[%1, %2]")
                                        .arg(entry.buyfundsinterval().from().value().c_str())
                                        .arg(entry.buyfundsinterval().to().value().c_str())
                                 << "sellInterval:"
                                 << QString("[%1, %2]")
                                        .arg(entry.sellfundsinterval().from().value().c_str())
                                        .arg(entry.sellfundsinterval().to().value().c_str())
                                 << "buyFee" << entry.buyfeepercent().c_str() << "sellFee"
                                 << entry.sellfeepercent().c_str();

            auto convert = [](auto interval) {
                return TradingPair::BalanceInterval{ ConvertBigInt(interval.from()),
                    ConvertBigInt(interval.to()) };
            };

            TradingPair tp;
            tp.pairId = entry.id();
            tp.buyFeePercent = QString::fromStdString(entry.buyfeepercent()).toDouble();
            tp.sellFeePercent = QString::fromStdString(entry.sellfeepercent()).toDouble();
            tp.buyFundsInterval = convert(entry.buyfundsinterval());
            tp.buyPriceInterval = convert(entry.buypriceinterval());
            tp.sellFundsInterval = convert(entry.sellfundsinterval());
            tp.sellPriceInterval = convert(entry.sellpriceinterval());

            result.emplace_back(tp);
        }
        return TradingPairInfo(result, pairs.paysfees());
    });
}

//==============================================================================

OrderbookApiClient::State OrderbookClient::state() const
{
    return _state;
}

//==============================================================================

TradingOrdersModel* OrderbookClient::ordersModel() const
{
    return _tradingOrdersModel;
}

//==============================================================================

OrderbookChannelRenting* OrderbookClient::renting() const
{
    return _channelRenting;
}

//==============================================================================

OrderbookRefundableFees* OrderbookClient::feeRefunding() const
{
    return _refundableFees;
}

//==============================================================================

Promise<void> OrderbookClient::sendOrderMessage(
    std::string orderId, std::vector<unsigned char> serialized)
{
    Command msgCommand;
    auto cmd = new stakenet::orderbook::protos::SendOrderMessageCommand;
    cmd->set_message(std::string(serialized.begin(), serialized.end()));
    cmd->set_orderid(orderId);
    msgCommand.set_allocated_sendordermessage(cmd);
    return _apiClient->makeRequest(msgCommand).then([](OrderbookResponse response) {
        auto r = response.sendordermessageresponse();
        if (r.has_matchedordernotfound()) {
            throw std::runtime_error("Matched order not found");
        }
    });
}

//==============================================================================

Promise<PlaceOrderOutcome> OrderbookClient::placeOrder(orderbook::OwnLimitOrder ownLimitOrder)
{
    std::string localId = generateId();
    ownLimitOrder.localId = localId;

    LogCDebug(Orderbook) << "Placing order:" << ownLimitOrder.pairId.c_str() << localId.data()
                         << ownLimitOrder.quantity << ownLimitOrder.price;

    OwnOrder ownOrder(ownLimitOrder,
        Stamp{ OrderIdentifier{ std::string{} }, QDateTime::currentMSecsSinceEpoch(),
            ownLimitOrder.quantity },
        true);

    addMyOrder(_myorders[ownOrder.pairId], ownOrder);

    return _apiClient->makeRequest(ConvertToPlaceOrderCmd(ownLimitOrder))
        .then([ownLimitOrder, localId, this](OrderbookResponse response) {
            auto placeOrderResp = response.placeorderresponse();

            switch (placeOrderResp.value_case()) {
            case stakenet::orderbook::protos::PlaceOrderResponse::ValueCase::kMyOrderPlaced: {
                auto order = placeOrderResp.myorderplaced().order();
                const auto price = ConvertBigInt(order.details().price());

                if (ownLimitOrder.isBuy) {
                    auto amount = swaps::MakerTakerAmounts::CalculateOrderAmount(
                        ownLimitOrder.quantity, price, ownLimitOrder.pairId);
                    // we need to apply this fix, cause we operate always on base currency in
                    // quantity
                    order.mutable_details()->set_allocated_funds(ConvertToBigInt(amount).release());
                }

                const auto quantity = ConvertBigInt(order.details().funds());

                OwnOrder ownOrder(
                    OwnLimitOrder{ LimitOrder{ MarketOrder{ quantity, ownLimitOrder.pairId,
                                                   ownLimitOrder.isBuy },
                                       price },
                        Local{ localId, 0 }, ownLimitOrder.feePaymentHash },
                    Stamp{ OrderIdentifier{ order.details().orderid() },
                        QDateTime::currentMSecsSinceEpoch(), quantity },
                    true);
                ownOrder.orderStatus = OwnOrderStatus::Confirmed;
                ownOrder.orderType = OwnOrderType::Limit;

                LogCDebug(Orderbook)
                    << "Order placed:" << ownOrder.pairId.c_str() << ownOrder.id.c_str();

                this->addMyOrder(_myorders[ownOrder.pairId], ownOrder);

                return PlaceOrderOutcome(ownOrder);
            }
            case stakenet::orderbook::protos::PlaceOrderResponse::ValueCase::kMyOrderMatched: {
                auto matchedTrade = placeOrderResp.myordermatched().trade();
                _peerPool->addSwapPeer(matchedTrade.existingorderid());

                auto price = ConvertBigInt(matchedTrade.price());
                auto baseQuantity = ownLimitOrder.isBuy
                    ? swaps::MakerTakerAmounts::CalculateOrderAmount(
                          ownLimitOrder.quantity, price, ownLimitOrder.pairId)
                    : ownLimitOrder.quantity;

                // if we are here we are taker
                OwnOrder ownOrder(OwnLimitOrder{ LimitOrder{ MarketOrder{ baseQuantity,
                                                                 matchedTrade.tradingpair(),
                                                                 ownLimitOrder.isBuy },
                                                     price },
                                      Local{
                                          ownLimitOrder.localId,
                                          0,
                                      },
                                      ownLimitOrder.feePaymentHash },
                    Stamp{ OrderIdentifier{ matchedTrade.id() },
                        QDateTime::currentMSecsSinceEpoch(), baseQuantity },
                    true);

                ownOrder.orderStatus = OwnOrderStatus::Confirmed;
                ownOrder.orderType
                    = ownLimitOrder.price > 0 ? OwnOrderType::Limit : OwnOrderType::Market;

                this->addMyOrder(_myorders[ownOrder.pairId], ownOrder);

                orderbook::Trade trade(matchedTrade.existingorderid(), price,
                    ConvertBigInt(matchedTrade.size()), ownOrder,
                    ConvertToPeerOrder(placeOrderResp.myordermatched()), matchedTrade.executedon());

                LogCDebug(Orderbook) << "Order matched:" << matchedTrade.id().c_str()
                                     << matchedTrade.existingorderid().c_str()
                                     << matchedTrade.executingorderid().c_str();

                _matchedOrders[matchedTrade.tradingpair()][matchedTrade.existingorderid()]
                    = MatchedTrade{ matchedTrade, boost::make_optional(ownOrder) };
                return PlaceOrderOutcome(trade);
            }
            case stakenet::orderbook::protos::PlaceOrderResponse::ValueCase::kMyOrderRejected: {
                auto myOrderRejected = placeOrderResp.myorderrejected();
                throw std::runtime_error(myOrderRejected.reason());
                break;
            }
            default:
                break;
            }

            LogCCritical(Orderbook)
                << "Received unexpected place order response:"
                << "Message id:" << response.clientmessageid().c_str()
                << "Command value_case:" << response.value_case()
                << "PlaceOrderResponse value_case:" << placeOrderResp.value_case();

            throw std::runtime_error("Invalid place order response");
        })
        .tapFail([this, pairId = ownLimitOrder.pairId, localId]() {
            auto ordersIt = _myorders.find(pairId);
            if (ordersIt != std::end(_myorders)) {
                auto it = findOrderById(ordersIt->second, pairId, localId);
                if (it != std::end(ordersIt->second)) {
                    LogCDebug(Swaps) << "Place order tapFail:" << localId.data();
                    this->eraseMyOrder(ordersIt->second, it);
                    this->ownOrderCanceled(localId);
                }
            }
        });
}

//==============================================================================

Promise<PlaceOrderOutcome> OrderbookClient::placeOrderPortion(
    std::string pairId, std::string baseLocalId, int64_t portion, OwnOrderType orderType)
{
    return QtPromise::attempt([=] { return getOwnOrder(pairId, baseLocalId); })
        .then([this, portion, baseLocalId, orderType](OwnOrder ownOrder) {
            LogCDebug(Orderbook) << "Placing partial order:" << ownOrder.pairId.c_str()
                                 << ownOrder.localId.c_str() << ownOrder.quantity << ownOrder.price;

            OwnLimitOrder ownLimitOrder(ownOrder, ownOrder, ownOrder.feePaymentHash);
            ownLimitOrder.quantity = portion;
            ownLimitOrder.price = orderType == orderbook::OwnOrderType::Market ? 0 : ownOrder.price;

            return _apiClient->makeRequest(ConvertToPlaceOrderCmd(ownLimitOrder))
                .then([ownLimitOrder, baseLocalId, ownOrder, this](OrderbookResponse response) {
                    auto placeOrderResp = response.placeorderresponse();

                    switch (placeOrderResp.value_case()) {
                    case stakenet::orderbook::protos::PlaceOrderResponse::ValueCase::
                        kMyOrderPlaced: {
                        auto order = placeOrderResp.myorderplaced().order();
                        const auto price = ConvertBigInt(order.details().price());

                        if (ownLimitOrder.isBuy) {
                            auto amount = swaps::MakerTakerAmounts::CalculateOrderAmount(
                                ownLimitOrder.quantity, price, ownLimitOrder.pairId);
                            // we need to apply this fix, cause we operate always on base currency
                            // in quantity
                            order.mutable_details()->set_allocated_funds(
                                ConvertToBigInt(amount).release());
                        }

                        OrderPortion portion{ ownLimitOrder.pairId, order.details().orderid(),
                            ConvertBigInt(order.details().funds()) };
                        addOrderPortion(ownLimitOrder.pairId, baseLocalId, portion);

                        LogCDebug(Orderbook)
                            << "Partial Order placed:" << ownLimitOrder.pairId.c_str()
                            << portion.orderId.c_str();
                        return PlaceOrderOutcome(portion);
                    }
                    case stakenet::orderbook::protos::PlaceOrderResponse::ValueCase::
                        kMyOrderMatched: {
                        auto matchedTrade = placeOrderResp.myordermatched().trade();
                        _peerPool->addSwapPeer(matchedTrade.existingorderid());

                        auto price = ConvertBigInt(matchedTrade.price());
                        auto baseQuantity = ownLimitOrder.isBuy
                            ? swaps::MakerTakerAmounts::CalculateOrderAmount(
                                  ownLimitOrder.quantity, price, ownLimitOrder.pairId)
                            : ownLimitOrder.quantity;

                        // if we are here we are taker
                        OwnOrder ownOrder(OwnLimitOrder{ LimitOrder{ MarketOrder{ baseQuantity,
                                                                         matchedTrade.tradingpair(),
                                                                         ownLimitOrder.isBuy },
                                                             price },
                                              Local{
                                                  std::string{},
                                                  0,
                                              },
                                              ownLimitOrder.feePaymentHash },
                            Stamp{ OrderIdentifier{ matchedTrade.id() },
                                QDateTime::currentMSecsSinceEpoch(), baseQuantity },
                            true);

                        orderbook::Trade trade(matchedTrade.existingorderid(), price,
                            ConvertBigInt(matchedTrade.size()), ownOrder,
                            ConvertToPeerOrder(placeOrderResp.myordermatched()),
                            matchedTrade.executedon());

                        OrderPortion portion{ ownLimitOrder.pairId, matchedTrade.existingorderid(),
                            baseQuantity };
                        addOrderPortion(ownLimitOrder.pairId, baseLocalId, portion);

                        LogCDebug(Orderbook) << "Order matched:" << matchedTrade.id().c_str()
                                             << matchedTrade.existingorderid().c_str()
                                             << matchedTrade.executingorderid().c_str();

                        _matchedOrders[matchedTrade.tradingpair()][matchedTrade.existingorderid()]
                            = MatchedTrade{ matchedTrade, boost::make_optional(ownOrder) };

                        return PlaceOrderOutcome(trade);
                    }
                    case stakenet::orderbook::protos::PlaceOrderResponse::ValueCase::
                        kMyOrderRejected: {
                        auto myOrderRejected = placeOrderResp.myorderrejected();
                        throw std::runtime_error(myOrderRejected.reason());
                        break;
                    }
                    default:
                        break;
                    }

                    LogCCritical(Orderbook)
                        << "Received unexpected place order response:"
                        << "Message id:" << response.clientmessageid().c_str()
                        << "Command value_case:" << response.value_case()
                        << "PlaceOrderResponse value_case:" << placeOrderResp.value_case();

                    throw std::runtime_error("Invalid place order response");
                });
        });
}

//==============================================================================

void OrderbookClient::ownOrders(std::function<void(const OwnOrder&)> func) const
{
    for (auto pair : _myorders) {
        if (_myorders.count(pair.first) > 0) {
            for (auto it : pair.second) {
                func(it.second);
            }
        }
    }
}

//==============================================================================

void OrderbookClient::ownOrdersByPairId(
    std::function<void(const OwnOrder&)> func, std::string pairId) const
{
    if (_myorders.count(pairId) > 0) {
        for (auto it : _myorders.at(pairId)) {
            func(it.second);
        }
    }
}

//==============================================================================

Promise<void> OrderbookClient::cancelOrder(std::string pairId, std::string orderId)
{
    return cancelOwnOrder(pairId, orderId).then([this, orderId] { ownOrderCanceled(orderId); });
}

//==============================================================================

void OrderbookClient::cancelOrders(std::string pairId)
{
    if (_myorders.count(pairId) > 0) {
        _myorders.at(pairId).clear();
        Command clearOrdersCommand;
        auto cmd = new io::stakenet::orderbook::protos::CleanTradingPairOrdersCommand;
        cmd->set_tradingpair(pairId);
        clearOrdersCommand.set_allocated_cleantradingpairorders(cmd);
        _apiClient->makeRequest(clearOrdersCommand);
        ownOrdersChanged(pairId);
    }
}

//==============================================================================

OrderbookEventDispatcher& OrderbookClient::dispatcher()
{
    return *_dispatcher;
}

//==============================================================================

OrderbookSwapPeerPool& OrderbookClient::peerPool()
{
    return *_peerPool;
}

//==============================================================================

void OrderbookClient::onOwnOrderSwapSuccess(swaps::SwapSuccess swap)
{
    if (auto baseOrder = tryGetBaseOrder(swap.pairId, swap.orderId)) {
        auto& ownOrder
            = findOrderById(_myorders.at(baseOrder->pairId), swap.pairId, baseOrder->localId)
                  ->second;
        auto it = std::find_if(std::begin(ownOrder.openPortions), std::end(ownOrder.openPortions),
            [id = swap.orderId](const auto& portion) { return id == portion.orderId; });

        if (it != std::end(ownOrder.openPortions)) {
            ownOrder.closedPortions.emplace_back(
                swap.pairId, it->orderId, ownOrder.isBuy ? swap.amountReceived : swap.amountSent);
            ownOrder.openPortions.erase(it);
            ownOrderChanged(ownOrder);
        }
    } else if (_myorders.count(swap.pairId) > 0) {
        auto& orders = _myorders.at(swap.pairId);
        auto it = findOrderById(orders, swap.pairId, swap.localId);
        if (it != std::end(orders)) {
            auto& ownOrder = it->second;
            ownOrder.closedPortions.emplace_back(
                swap.pairId, swap.localId, ownOrder.isBuy ? swap.amountReceived : swap.amountSent);
            ownOrderChanged(ownOrder);
        }
    }

    removeMatchedOrder(swap.pairId, swap.orderId);
}

//==============================================================================

void OrderbookClient::onOwnOrderSwapFailure(std::string pairId, const swaps::SwapFailure& failure)
{
    removeMatchedOrder(pairId, failure.orderId);
    if (auto baseOrderOpt = tryGetBaseOrder(pairId, failure.orderId)) {
        auto& baseOrder = findOrderById(
            _myorders.at(baseOrderOpt->pairId), baseOrderOpt->pairId, baseOrderOpt->id)
                              ->second;
        auto it = std::find_if(std::begin(baseOrder.openPortions), std::end(baseOrder.openPortions),
            [id = failure.orderId](const auto& portion) { return id == portion.orderId; });
        if (it != std::end(baseOrder.openPortions)) {
            baseOrder.openPortions.erase(it);
            ownOrderChanged(baseOrder);
        }
    } else {
        LogCDebug(Swaps) << "Swap failed, removing own order:" << failure.localId.data();
        removeOwnOrder(pairId, failure.localId);
    }
}

//==============================================================================

void OrderbookClient::onOwnOrderCompleted(const OwnOrder& ownOrder)
{
    LogCDebug(Swaps) << "Swap completed, removing own order:" << ownOrder.localId.data();
    removeOwnOrder(ownOrder.pairId, ownOrder.localId);
    ownOrderCompleted(ownOrder.localId);
}

//==============================================================================

void OrderbookClient::onOwnOrderMatched(Event::ServerEvent event)
{
    auto matchedTrade = event.myordermatched().trade();
    try {
        LogCDebug(Swaps) << "Own order matched:" << matchedTrade.existingorderid().data()
                         << matchedTrade.executingorderid().data();
        _peerPool->addSwapPeer(matchedTrade.executingorderid());

        // if we are here we are maker
        auto ownOrder = getOwnOrder(matchedTrade.tradingpair(), matchedTrade.existingorderid());
        _matchedOrders[matchedTrade.tradingpair()][matchedTrade.existingorderid()]
            = std::make_tuple(matchedTrade, boost::make_optional(ownOrder));
    } catch (std::exception& ex) {
        LogCCritical(Swaps) << "Failed to process order as maker" << ex.what()
                            << matchedTrade.tradingpair().c_str()
                            << matchedTrade.executingorderid().c_str()
                            << matchedTrade.existingorderid().c_str();
    }
}

//==============================================================================

void OrderbookClient::onConnectedToOrderbook()
{
    if (_subscriptions.empty()) {
        setState(OrderbookApiClient::State::Connected);
    } else {
        QtPromise::map(_subscriptions,
            [this](auto pairId, ...) {
                return _tradingOrdersModel->subscribe(pairId).then([] { return true; });
            })
            .then([this] { setState(OrderbookApiClient::State::Connected); })
            .fail([this] { _apiClient->reconnect(); });
    }
}

//==============================================================================

void OrderbookClient::onApiClientStateChanged(OrderbookApiClient::State newState)
{
    if (newState == OrderbookApiClient::State::InMaintenance) {
        setState(newState);
    } else if (newState == OrderbookApiClient::State::Connected) {
        onConnectedToOrderbook();
    }
}

//==============================================================================

void OrderbookClient::setState(OrderbookApiClient::State state)
{
    if (_state != state) {
        _state = state;
        stateChanged(state);
    }
}

//==============================================================================

void OrderbookClient::init()
{
    qRegisterMetaType<OwnOrder>("OwnOrder");
    qRegisterMetaType<LimitOrder>("LimitOrder");

    _apiClient->setParent(this);
    connect(
        _apiClient, &OrderbookApiClient::connected, this, &OrderbookClient::onConnectedToOrderbook);
    connect(_apiClient, &OrderbookApiClient::disconnected, this, [this] {
        if (_state != OrderbookApiClient::State::Error) {
            setState(OrderbookApiClient::State::Disconnected);
        }
    });
    connect(_apiClient, &OrderbookApiClient::stateChanged, this,
        &OrderbookClient::onApiClientStateChanged);
    connect(_apiClient, &OrderbookApiClient::connectionError, this,
        std::bind(&OrderbookClient::setState, this, OrderbookApiClient::State::Error));

    _channelRenting = new OrderbookChannelRenting(*_apiClient, this);
    _refundableFees = new OrderbookRefundableFees(*_apiClient, this);
    _tradingOrdersModel = new TradingOrdersModel(*_apiClient, *_dispatcher, this);

    _dispatcher->addEventHandler(Event::ServerEvent::ValueCase::kMyOrderMatched,
        std::bind(&OrderbookClient::onOwnOrderMatched, this, std::placeholders::_1));

    connect(_tradingOrdersModel, &TradingOrdersModel::orderAdded, this,
        [this](const auto& pairId, const auto& details, auto isAsk) {
            this->orderAdded(
                LimitOrder{ MarketOrder{ details.amount, pairId, !isAsk }, details.price });
        });
    connect(_tradingOrdersModel, &TradingOrdersModel::orderRemoved, this,
        [this](const auto& pairId, const auto& details, auto isAsk) {
            this->orderRemoved(
                LimitOrder{ MarketOrder{ details.amount, pairId, !isAsk }, details.price });
        });
}

//==============================================================================

Promise<void> OrderbookClient::cancelOwnOrder(std::string pairId, std::string localId)
{
    if (_myorders.count(pairId) > 0) {

        std::string orderId;
        std::vector<std::string> ordersToCancel;
        {
            auto baseOrder = tryGetBaseOrder(pairId, localId);
            auto ownOrder = tryGetOwnOrder(pairId, localId);
            if (baseOrder || ownOrder) {
                baseOrder = baseOrder ? baseOrder : ownOrder;
                orderId = baseOrder->id;
                std::transform(std::begin(baseOrder->openPortions),
                    std::end(baseOrder->openPortions), std::back_inserter(ordersToCancel),
                    [](const auto& portion) { return portion.orderId; });
            } else {
                return Promise<void>::resolve();
            }
        }

        if (tryGetMatchedTrade(pairId, orderId)) {
            return Promise<void>::reject(
                std::runtime_error("Can't cancel order which is in progress"));
        }

        ordersToCancel.emplace_back(orderId);

        auto& ownOrders = _myorders.at(pairId);

        std::for_each(std::begin(ordersToCancel), std::end(ordersToCancel),
            [&ownOrders, &pairId, &localId, this](const auto& orderId) {
                auto it = this->findOrderById(ownOrders, pairId, orderId);
                if (it != std::end(ownOrders)) {
                    LogCDebug(Swaps) << "Own order canceled:" << localId.data() << orderId.data()
                                     << "erasing from own orders";
                    this->eraseMyOrder(ownOrders, it);
                }
            });

        return QtPromise::each(ordersToCancel,
            [this](const std::string& orderId, ...) { return cancelRemoteOwnOrder(orderId); })
            .then([] {});

    } else {
        return Promise<void>::reject(
            std::runtime_error(strprintf("Can't cancel order with current pair id: %s", pairId)));
    }
}

//==============================================================================

Promise<void> OrderbookClient::cancelRemoteOwnOrder(std::string orderId)
{
    Command cancelCommand;
    auto* cmd = new stakenet::orderbook::protos::CancelOpenOrderCommand;
    cmd->set_orderid(orderId);
    cancelCommand.set_allocated_cancelorder(cmd);
    return _apiClient->makeRequest(cancelCommand).then([] {});
}

//==============================================================================

auto OrderbookClient::findOrderById(const OrderbookClient::MyOrders& where, std::string pairId,
    std::string orderId) const -> MyOrders::const_iterator
{
    auto it = where.find(orderId);
    if (it == std::cend(where)) {
        if (_orderIdToLocalIdIndex.count(pairId) > 0) {
            const auto& index = _orderIdToLocalIdIndex.at(pairId);
            auto lIt = index.find(orderId);
            if (lIt != std::cend(index)) {
                it = where.find(lIt->second);
            }
        }
    }

    return it;
}

//==============================================================================

auto OrderbookClient::findOrderById(MyOrders& where, std::string pairId, std::string orderId) const
    -> MyOrders::iterator
{
    auto it = where.find(orderId);
    if (it == std::end(where)) {
        if (_orderIdToLocalIdIndex.count(pairId) > 0) {
            const auto& index = _orderIdToLocalIdIndex.at(pairId);
            auto lIt = index.find(orderId);
            if (lIt != std::end(index)) {
                it = where.find(lIt->second);
            }
        }
    }

    return it;
}

//==============================================================================

void OrderbookClient::eraseMyOrder(
    OrderbookClient::MyOrders& where, const MyOrders::const_iterator& it)
{
    auto localId = it->first;
    auto orderId = it->second.id;
    auto pairId = it->second.pairId;
    LogCDebug(Swaps) << "Removing own order:" << localId.data() << orderId.data();
    where.erase(it);
    auto ordersIt = _orderIdToLocalIdIndex.find(pairId);
    if (ordersIt != std::end(_orderIdToLocalIdIndex)) {
        ordersIt->second.erase(orderId);
    }
}

//==============================================================================

void OrderbookClient::addMyOrder(OrderbookClient::MyOrders& where, OwnOrder ownOrder)
{
    LogCDebug(Swaps) << "Adding own order:" << ownOrder.id.data() << ownOrder.localId.data();
    bool isUpdate = where.count(ownOrder.localId) > 0;

    where[ownOrder.localId] = ownOrder;
    if (!ownOrder.id.empty()) {
        _orderIdToLocalIdIndex[ownOrder.pairId][ownOrder.id] = ownOrder.localId;
    }

    if (isUpdate) {
        ownOrderChanged(ownOrder);
    } else {
        ownOrderPlaced(ownOrder);
    }
}

//==============================================================================

std::string OrderbookClient::generateId() const
{
    std::string id;
    do {
        id = QUuid::createUuid().toString().toStdString();
    } while (_myorders.count(id) > 0);

    return id;
}

//==============================================================================

void OrderbookClient::removeOwnOrder(std::string pairId, std::string orderId)
{
    if (_myorders.count(pairId) == 0)
        return;

    auto& myOrders = _myorders.at(pairId);

    auto it = findOrderById(myOrders, pairId, orderId);

    if (it != std::end(myOrders)) {
        auto order = it->second;
        auto localId = order.localId;
        if (_partialOrdersIndex.count(pairId) > 0) {
            auto& portionsIndex = _partialOrdersIndex.at(pairId);
            for (auto&& portion : order.openPortions) {
                portionsIndex.erase(portion.orderId);
            }
        }

        LogCDebug(Swaps) << "requested to remove own order" << orderId.data();
        eraseMyOrder(myOrders, it);
        ownOrderMatched(localId);
    }
}

//==============================================================================

void OrderbookClient::removeMatchedOrder(std::string pairId, std::string orderId)
{
    if (_matchedOrders.count(pairId) > 0) {
        _matchedOrders.at(pairId).erase(orderId);
    }
}

//==============================================================================

void OrderbookClient::addOrderPortion(
    std::string pairId, std::string baseLocalId, OrderPortion portion)
{
    auto& ownBaseOrder = _myorders.at(pairId).at(baseLocalId);
    ownBaseOrder.openPortions.emplace_back(portion);
    _partialOrdersIndex[pairId][portion.orderId] = baseLocalId;
    ownOrderChanged(ownBaseOrder);
}

//==============================================================================

int64_t ConvertBigInt(io::stakenet::orderbook::protos::BigInteger bigInt)
{
    auto result = std::stoll(bigInt.value());
    return result;
}

//==============================================================================

std::unique_ptr<io::stakenet::orderbook::protos::BigInteger> ConvertToBigInt(int64_t value)
{
    std::unique_ptr<io::stakenet::orderbook::protos::BigInteger> result(
        new io::stakenet::orderbook::protos::BigInteger);
    result->set_value(std::to_string(value));
    return result;
}

//==============================================================================
}
