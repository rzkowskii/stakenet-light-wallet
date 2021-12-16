#include "MovingGridStrategy.hpp"
#include <Utils/Logging.hpp>

#include <cmath>

namespace tradingbot {

//==============================================================================

MovingGridStrategy::MovingGridStrategy(
    Config cfg, std::unique_ptr<gateway::ExchangeGateway> gateway, QObject* parent)
    : QObject(parent)
    , _cfg(cfg)
    , _gateway(std::move(gateway))
{
    connect(_gateway.get(), &gateway::ExchangeGateway::orderCompleted, this,
        &MovingGridStrategy::onOrderCompleted);
    connect(_gateway.get(), &gateway::ExchangeGateway::orderChanged, this,
        &MovingGridStrategy::onOrderChanged);
}

//==============================================================================

Promise<void> MovingGridStrategy::start()
{
    if (_orders.empty()) {
        return initializeGrid();
    }

    return Promise<void>::resolve();
}

//==============================================================================

Promise<void> MovingGridStrategy::stop()
{
    std::vector<Promise<void>> promises;
    for (const auto& order : _orders) {
        promises.emplace_back(cancelOrder(order.first));
    }

    return QtPromise::all(promises).tap([this] { _orders.clear(); });
}

//==============================================================================

double MovingGridStrategy::gridPrice(double basePrice, bool isBuy, uint32_t level) const
{
    double base = 1 + _cfg.interval;
    return basePrice * std::pow(isBuy ? (1.0 / base) : base, level);
}

//==============================================================================

void MovingGridStrategy::onOrderPlaced(
    bool isBuy, uint32_t level, gateway::PlaceOrderOutcome outcome)
{
    if (auto executedOrder = std::get_if<gateway::ExecutedOwnOrder>(&outcome)) {
        LogCDebug(TradingBot) << (isBuy ? "buy" : "sell") << "order placed as taker:" << level
                              << executedOrder->price;
        if (auto ownOrder = _gateway->tryGetOwnOrder(
                executedOrder->pairId.toStdString(), executedOrder->localId.toStdString())) {
            _orders.emplace(
                ownOrder->localId, std::pair<uint32_t, orderbook::OwnOrder>{ level, *ownOrder });
        } else {
            LogCDebug(TradingBot) << "Could not get own order when it was executed as taker"
                                  << executedOrder->pairId << executedOrder->orderId;
        }
    } else if (auto ownOrder = std::get_if<orderbook::OwnOrder>(&outcome)) {
        LogCDebug(TradingBot) << (isBuy ? "buy" : "sell") << "order placed as maker:" << level
                              << ownOrder->price;
        // order placed, await for signals to make progress
        _orders.emplace(
            ownOrder->localId, std::pair<uint32_t, orderbook::OwnOrder>{ level, *ownOrder });
    }
}

//==============================================================================

void MovingGridStrategy::onOrderCompleted(std::string orderId)
{
    if (_orders.count(orderId) == 0) {
        return;
    }

    auto [level, ownOrder] = _orders.at(orderId);
    _orders.erase(orderId);
    LogCCritical(TradingBot) << ownOrder.pairId.data() << (ownOrder.isBuy ? "buy" : "sell")
                             << "order on level" << level << "price" << ownOrder.price
                             << "completed";

    placeOrder(!ownOrder.isBuy, 1);
}

//==============================================================================

void MovingGridStrategy::onOrderChanged(orderbook::OwnOrder ownOrder)
{
    auto it = _orders.find(ownOrder.localId);
    if (it != std::end(_orders)) {
        it->second.second = ownOrder;
    }
}

//==============================================================================

Promise<void> MovingGridStrategy::initializeGrid()
{
    std::vector<Promise<void>> promises;
    // buy orders
    for (uint32_t level = 1; level <= _cfg.levels; ++level) {
        promises.emplace_back(placeOrder(true, level));
    }

    // sell orders
    for (int32_t level = _cfg.levels; level > 0; --level) {
        promises.emplace_back(placeOrder(false, level));
    }
    return QtPromise::all(promises);
}

//==============================================================================

uint32_t MovingGridStrategy::calculateQuantity(bool isBuy) const
{
    auto amount = isBuy ? (_cfg.quoteQuantity / _cfg.levels) : (_cfg.baseQuantity / _cfg.levels);
    auto orderFee = amount / 100 * _cfg.orderFee;
    return amount - orderFee;
}

//==============================================================================

Promise<void> MovingGridStrategy::placeOrder(bool isBuy, uint32_t level)
{
    auto quantity = calculateQuantity(isBuy);

    auto marketPrice = _gateway->getLastPrice(_cfg.pairId);
    auto basePrice = gridPrice(marketPrice, isBuy, level);
    auto priceDiff = marketPrice - basePrice;

    // counting here extra risk parameter
    auto price = qRound((basePrice + (priceDiff / 100 * _cfg.risk)) * std::pow(10, 8));
    LogCDebug(TradingBot) << "Placing order:" << level << (isBuy ? "buy" : "sell")
                          << "base price:" << basePrice << "price:" << price;
    orderbook::LimitOrder order(
        orderbook::MarketOrder(quantity, _cfg.pairId.toStdString(), isBuy), price);
    return _gateway->placeOrder(order)
        .then([isBuy, level, this](
                  gateway::PlaceOrderOutcome outcome) { onOrderPlaced(isBuy, level, outcome); })
        .tapFail([](std::exception& ex) {
            LogCCritical(TradingBot) << "could not place order with error:" << ex.what();
        })
        .tapFail([] { LogCCritical(TradingBot) << "could not place order with unknown error"; });
}

//==============================================================================

Promise<void> MovingGridStrategy::cancelOrder(std::string localId)
{
    return _gateway->cancelOrder(_cfg.pairId.toStdString(), localId)
        .tapFail([](std::exception& ex) {
            LogCCritical(TradingBot) << "could not cancel order with error:" << ex.what();
        })
        .tapFail([] { LogCCritical(TradingBot) << "could not cancel order with unknown error"; });
}

//==============================================================================
}
