#ifndef TRADINGORDERSMODEL_HPP
#define TRADINGORDERSMODEL_HPP

#include <Orderbook/OrderbookApiClient.hpp>
#include <Orderbook/OrderbookEventDispatcher.hpp>
#include <QMetaType>
#include <QObject>
#include <boost/optional.hpp>

namespace orderbook {

class TradingOrdersModel : public QObject {
    Q_OBJECT
public:
    explicit TradingOrdersModel(OrderbookApiClient& client, OrderbookEventDispatcher& dispatcher,
        QObject* parent = nullptr);

    struct OrderSummary {
        int64_t price{ 0 };
        int64_t amount{ 0 };

        OrderSummary& operator+=(const OrderSummary& rhs);
        OrderSummary& operator-=(const OrderSummary& rhs);
    };

    using Details = OrderSummary;
    using HistoricTrade = io::stakenet::orderbook::protos::Trade;
    struct Trade {
        std::string id;
        std::string tradingPair;
        Details existingOrder;
        int64_t executedOn;
    };

    using Asks = std::map<int64_t, Details>;
    using Bids = std::map<int64_t, Details, std::greater<int64_t>>;
    using Trades = std::unordered_map<std::string, Trade>;
    using MyOrders = std::vector<io::stakenet::orderbook::protos::Order>;
    using HistoricTrades = std::vector<io::stakenet::orderbook::protos::Trade>;

    Promise<void> subscribe(std::string tradingPair);
    Promise<void> unsubsribe(std::string tradingPair);

    using Traverser = std::function<void(Details)>;

    void traverseAsks(
        std::string pairId, int64_t lastKnownPrice, uint32_t limit, Traverser func) const;
    void traverseBids(
        std::string pairId, int64_t lastKnownPrice, uint32_t limit, Traverser func) const;

    bool canFetchMore(std::string pairId) const;
    Promise<void> fetchMoreHistoricTrades(std::string pairId);
    HistoricTrades historicTrades(std::string pairId, std::string lastSeenId) const;

    const Asks& askOrders(std::string pairId) const;
    const Bids& bidOrders(std::string pairId) const;

    boost::optional<Trade> activeTrade(std::string pairId, std::string id) const;

public slots:

signals:
    void orderAdded(std::string pairId, Details details, bool isAsk);
    void orderRemoved(std::string pairId, Details details, bool isAsk);
    void ordersChanged(std::string pairId);
    void orderCanceled();
    void historicTradeAdded(std::string pairId, HistoricTrade trade);

private slots:
    void onOrderPlaced(Event::ServerEvent event);
    void onOrderMatched(Event::ServerEvent event);
    void onOrderCanceled(Event::ServerEvent event);

private:
    Promise<void> fetchHistoricTrades(
        std::string pairId, std::string lastSeenTradeId, int32_t limit = 100);
    void addHistoricTrade(io::stakenet::orderbook::protos::Trade trade);

private:
    OrderbookApiClient& _client;
    OrderbookEventDispatcher& _dispatcher;

    std::unordered_map<std::string, Asks> _activeAsks;
    std::unordered_map<std::string, Bids> _activeBids;
    std::unordered_map<std::string, HistoricTrades> _historicTrades;
    std::set<std::string> _fetchedAllHistoricTrades;
};
}

#endif // TRADINGORDERSMODEL_HPP
