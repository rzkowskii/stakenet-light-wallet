#include "TradingOrdersModel.hpp"
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/Types.hpp>
#include <Swaps/Types.hpp>
#include <Utils/Logging.hpp>

namespace orderbook {

//==============================================================================

template <typename Container, typename F>
static void TraverseHelper(const Container& where, uint64_t lastKey, uint32_t limit, F func)
{
    auto it = lastKey > 0 ? std::begin(where) : where.lower_bound(lastKey);
    while (it != std::end(where) && --limit > 0) {
        func(it->second);
        ++it;
    }
};

//==============================================================================

static TradingOrdersModel::Details ConvertToOrderSummary(
    const io::stakenet::orderbook::protos::Order& order)
{
    auto isBuy
        = order.side() == io::stakenet::orderbook::protos::Order::OrderSide::Order_OrderSide_buy;

    // Funds returned from orderbook are calculated as total in quote currency for buy orders,
    // should be always in base currency, so we convert
    TradingOrdersModel::Details summary;
    summary.price = ConvertBigInt(order.details().price());
    summary.amount = ConvertBigInt(order.details().funds());

    if (isBuy) {
        auto funds = swaps::MakerTakerAmounts::CalculateOrderAmount(
            summary.amount, summary.price, order.tradingpair());

        summary.amount = funds;
    }

    return summary;
}

//==============================================================================

TradingOrdersModel::TradingOrdersModel(
    OrderbookApiClient& client, OrderbookEventDispatcher& dispatcher, QObject* parent)
    : QObject(parent)
    , _client(client)
    , _dispatcher(dispatcher)
{
    qRegisterMetaType<Details>("Details");
    qRegisterMetaType<HistoricTrade>("HistoricTrade");
    _dispatcher.addEventHandler(Event::ServerEvent::ValueCase::kOrderPlaced,
        std::bind(&TradingOrdersModel::onOrderPlaced, this, std::placeholders::_1));
    _dispatcher.addEventHandler(Event::ServerEvent::ValueCase::kOrdersMatched,
        std::bind(&TradingOrdersModel::onOrderMatched, this, std::placeholders::_1));
    _dispatcher.addEventHandler(Event::ServerEvent::kOrderCanceled,
        std::bind(&TradingOrdersModel::onOrderCanceled, this, std::placeholders::_1));
}

//==============================================================================

Promise<void> TradingOrdersModel::subscribe(std::string tradingPair)
{
    LogCDebug(Orderbook) << "Subscribing for" << tradingPair.c_str();
    Command subscribeCmd;
    auto cmd = new io::stakenet::orderbook::protos::SubscribeCommand;
    cmd->set_tradingpair(tradingPair);
    cmd->set_retrieveorderssummary(true);
    subscribeCmd.set_allocated_subscribe(cmd);
    QPointer<TradingOrdersModel> self(this);
    return _client.makeRequest(subscribeCmd).then([self, tradingPair](OrderbookResponse response) {
        if (!self) {
            return;
        }

        auto& asks = self->_activeAsks[tradingPair];
        auto& bids = self->_activeBids[tradingPair];

        asks.clear();
        bids.clear();

        auto copyHelper = [](const auto& from, auto& to) {
            for (auto&& item : from) {
                OrderSummary summary;
                summary.price = ConvertBigInt(item.price());
                summary.amount = ConvertBigInt(item.amount());
                to.emplace(ConvertBigInt(item.price()), summary);
            }
        };

        auto subscribeResponse = response.subscriberesponse();

        copyHelper(subscribeResponse.summaryasks(), asks);
        copyHelper(subscribeResponse.summarybids(), bids);

        for (auto& bid : bids) {
            auto funds = swaps::MakerTakerAmounts::CalculateOrderAmount(
                bid.second.amount, bid.second.price, tradingPair);

            bid.second.amount = funds;
        }

        LogCDebug(Orderbook) << "Got asks:" << asks.size() << "got bids:" << bids.size()
                             << "trades for" << tradingPair.c_str();
        self->ordersChanged(tradingPair);
    });
}

//==============================================================================

Promise<void> TradingOrdersModel::unsubsribe(std::string tradingPair)
{
    LogCDebug(Orderbook) << "Unsubscribing from" << tradingPair.c_str();
    Command unsubscribeCmd;
    auto cmd = new io::stakenet::orderbook::protos::UnsubscribeCommand;
    cmd->set_tradingpair(tradingPair);
    unsubscribeCmd.set_allocated_unsubscribe(cmd);
    return _client.makeRequest(unsubscribeCmd).then([] {});
}

//==============================================================================

void TradingOrdersModel::traverseAsks(
    std::string pairId, int64_t lastKnownPrice, uint32_t limit, Traverser func) const
{
    if (_activeAsks.count(pairId) > 0) {
        TraverseHelper(_activeAsks.at(pairId), lastKnownPrice, limit, func);
    }
}

//==============================================================================

void TradingOrdersModel::traverseBids(
    std::string pairId, int64_t lastKnownPrice, uint32_t limit, Traverser func) const
{
    if (_activeBids.count(pairId) > 0) {
        TraverseHelper(_activeBids.at(pairId), lastKnownPrice, limit, func);
    }
}

//==============================================================================

bool TradingOrdersModel::canFetchMore(std::string pairId) const
{
    return _fetchedAllHistoricTrades.count(pairId) == 0;
}

//==============================================================================

Promise<void> TradingOrdersModel::fetchMoreHistoricTrades(std::string pairId)
{
    const auto& trades = _historicTrades[pairId];
    return fetchHistoricTrades(pairId, trades.empty() ? std::string() : trades.back().id());
}

//==============================================================================

TradingOrdersModel::HistoricTrades TradingOrdersModel::historicTrades(
    std::string pairId, std::string lastSeenId) const
{
    HistoricTrades result;
    if (_historicTrades.count(pairId) > 0) {
        const auto& trades = _historicTrades.at(pairId);
        auto it = std::find_if(std::begin(trades), std::end(trades),
            [lastSeenId](const auto& trade) { return trade.id() == lastSeenId; });

        if (it == std::end(trades)) {
            if (lastSeenId.empty()) {
                it = std::begin(trades);
            }
        } else {
            // we don't want to include lastSeenId
            std::advance(it, 1);
        }

        std::copy(it, std::end(trades), std::back_inserter(result));
    }

    return result;
}

//==============================================================================

const TradingOrdersModel::Asks& TradingOrdersModel::askOrders(std::string pairId) const
{
    static Asks empty;
    return _activeAsks.count(pairId) > 0 ? _activeAsks.at(pairId) : empty;
}

//==============================================================================

const TradingOrdersModel::Bids& TradingOrdersModel::bidOrders(std::string pairId) const
{
    static Bids empty;
    return _activeBids.count(pairId) > 0 ? _activeBids.at(pairId) : empty;
}

//==============================================================================

Promise<void> TradingOrdersModel::fetchHistoricTrades(
    std::string pairId, std::string lastSeenTradeId, int32_t limit)
{
    if (!canFetchMore(pairId)) {
        return QtPromise::resolve();
    }

    Command getHistoricTradesCmd;
    auto cmd = new io::stakenet::orderbook::protos::GetHistoricTradesCommand;
    cmd->set_limit(limit);
    cmd->set_tradingpair(pairId);
    cmd->set_lastseentradeid(lastSeenTradeId);
    getHistoricTradesCmd.set_allocated_gethistorictrades(cmd);
    return _client.makeRequest(getHistoricTradesCmd)
        .then([this, pairId, lastSeenTradeId](OrderbookResponse response) {
            auto historicTradesResponse = response.gethistorictradesresponse();
            if (historicTradesResponse.trades_size() == 0) {
                _fetchedAllHistoricTrades.insert(pairId);
                return;
            }

            auto& trades = historicTradesResponse.trades();
            auto& tradesDest = _historicTrades[pairId];
            auto firstId = trades.begin()->id();

            auto it = std::find_if(std::begin(tradesDest), std::end(tradesDest),
                [firstId](const auto& trade) { return trade.id() == firstId; });

            if (it != std::end(tradesDest)) {
                return;
            }

            for (auto&& trade : trades) {
                tradesDest.emplace_back(trade);
            }
        });
}

//==============================================================================

void TradingOrdersModel::addHistoricTrade(io::stakenet::orderbook::protos::Trade trade)
{
    _historicTrades[trade.tradingpair()].emplace_back(trade);
    emit historicTradeAdded(trade.tradingpair(), trade);
}

//==============================================================================

void TradingOrdersModel::onOrderPlaced(Event::ServerEvent event)
{
    auto order = event.orderplaced().order();
    auto isBuy
        = order.side() == io::stakenet::orderbook::protos::Order::OrderSide::Order_OrderSide_buy;

    LogCDebug(Orderbook) << "Order placed:" << order.tradingpair().c_str()
                         << order.details().orderid().c_str() << isBuy;
    auto summary = ConvertToOrderSummary(order);

    if (isBuy) {
        _activeBids[order.tradingpair()][summary.price] += summary;
    } else {
        _activeAsks[order.tradingpair()][summary.price] += summary;
    }

    orderAdded(order.tradingpair(), summary, !isBuy);
}

//==============================================================================

void TradingOrdersModel::onOrderMatched(Event::ServerEvent event)
{
    auto trade = event.ordersmatched().trade();

    auto deleteOrder = [this, trade](auto& where, bool isAsk) {
        auto pairId = trade.tradingpair();
        if (where.count(pairId) > 0) {
            auto& orders = where.at(pairId);
            auto it = orders.find(ConvertBigInt(trade.price()));
            if (it != std::end(orders)) {
                OrderSummary summary;
                summary.price = ConvertBigInt(trade.price());
                summary.amount = ConvertBigInt(trade.existingorderfunds());

                if (!isAsk) {
                    auto funds = swaps::MakerTakerAmounts::CalculateOrderAmount(
                        summary.amount, summary.price, pairId);
                    summary.amount = funds;
                }

                it->second -= summary;
                if (it->second.amount <= 0) {
                    orders.erase(it);
                }

                this->orderRemoved(pairId, summary, isAsk);
            }
        }
    };

    if (QString::fromStdString(trade.executingorderside()).toLower() == "sell") {
        deleteOrder(_activeBids, false);
    } else {
        deleteOrder(_activeAsks, true);
    }

    addHistoricTrade(trade);
}

//==============================================================================

void TradingOrdersModel::onOrderCanceled(Event::ServerEvent event)
{
    auto order = event.ordercanceled().order();
    auto deleteOrder = [this, summary = ConvertToOrderSummary(order), pairId = order.tradingpair()](
                           auto& where, bool isAsk) {
        if (where.count(pairId) > 0) {
            auto& orders = where.at(pairId);
            auto it = orders.find(summary.price);
            if (it != std::end(orders)) {
                it->second -= summary;
                if (it->second.amount <= 0) {
                    orders.erase(it);
                }

                this->orderRemoved(pairId, summary, isAsk);
                return true;
            }
        }

        return false;
    };

    auto isBuy
        = order.side() == io::stakenet::orderbook::protos::Order::OrderSide::Order_OrderSide_buy;

    if (isBuy) {
        deleteOrder(_activeBids, false);
    } else {
        deleteOrder(_activeAsks, true);
    }
}

//==============================================================================

TradingOrdersModel::OrderSummary& TradingOrdersModel::OrderSummary::operator+=(
    const OrderSummary& rhs)
{
    this->amount += rhs.amount;
    return *this;
}

//==============================================================================

TradingOrdersModel::OrderSummary& TradingOrdersModel::OrderSummary::operator-=(
    const OrderSummary& rhs)
{
    this->amount -= rhs.amount;
    return *this;
}

//==============================================================================
}
