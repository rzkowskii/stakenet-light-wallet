#ifndef ORDERBOOKCLIENT_HPP
#define ORDERBOOKCLIENT_HPP

#include <QObject>
#include <memory>
#include <queue>
#include <unordered_map>

#include <Orderbook/AbstractOrderbookClient.hpp>
#include <Orderbook/OrderbookApiClient.hpp>
#include <Swaps/Types.hpp>
#include <Utils/Utils.hpp>

namespace orderbook {

class OrderbookEventDispatcher;
class OrderbookSwapPeerPool;
class TradingOrdersModel;
class OrderbookChannelRenting;
class OrderbookRefundableFees;

class OrderbookClient : public AbstractOrderbookClient {
    Q_OBJECT
public:
    using MatchedTrade
        = std::tuple<io::stakenet::orderbook::protos::Trade, boost::optional<OwnOrder>>;
    using TradingPairInfo = std::tuple<std::vector<TradingPair>, bool>;

    explicit OrderbookClient(std::unique_ptr<OrderbookApiClient> client, QObject* parent = nullptr);
    ~OrderbookClient() override;

    boost::optional<OwnOrder> tryGetOwnOrder(std::string pairId, std::string orderId) const;
    OwnOrder getOwnOrder(std::string pairId, std::string orderId) const override;
    boost::optional<MatchedTrade> tryGetMatchedTrade(std::string pairId, std::string orderId);
    boost::optional<OwnOrder> tryGetOrderPortion(
        std::string pairId, std::string partialOrderId) const;
    // accepts only global orderbook identifier
    boost::optional<OwnOrder> tryGetBaseOrder(std::string pairId, std::string partialOrderId) const;

    void start();

    Promise<void> registerLndKey(std::string currency, std::string pubkey);
    Promise<void> registerConnextKey(std::string currency, std::string pubkey);
    Promise<void> subscribe(std::string pairId);
    Promise<void> unsubscribe(std::string pairId);
    std::vector<std::string> activatedPairs() const;

    Promise<void> isPairActivated(std::string pairId);
    Promise<TradingPairInfo> tradingPairs();

    TradingOrdersModel* ordersModel() const;
    OrderbookChannelRenting* renting() const;
    OrderbookRefundableFees* feeRefunding() const;

    OrderbookApiClient::State state() const;

    Promise<void> sendOrderMessage(std::string orderId, std::vector<unsigned char> serialized);

    Promise<PlaceOrderOutcome> placeOrder(orderbook::OwnLimitOrder ownOrder);
    Promise<PlaceOrderOutcome> placeOrderPortion(std::string pairId, std::string baseLocalId,
        int64_t portion, orderbook::OwnOrderType orderType);

    void ownOrders(std::function<void(const OwnOrder&)> func) const;
    void ownOrdersByPairId(std::function<void(const OwnOrder&)> func, std::string pairId) const;

    Promise<void> cancelOrder(std::string pairId, std::string orderId);
    void cancelOrders(std::string pairId);

    OrderbookEventDispatcher& dispatcher();
    OrderbookSwapPeerPool& peerPool();

signals:
    void stateChanged(OrderbookApiClient::State newState);
    void orderAdded(LimitOrder order);
    void orderRemoved(LimitOrder order);
    void ownOrderPlaced(OwnOrder ownOrder);
    void ownOrderChanged(OwnOrder ownOrder);
    void ownOrderCompleted(std::string orderId);
    void ownOrderMatched(std::string orderId);
    void ownOrderCanceled(std::string orderId);
    void ownOrdersChanged(std::string pairId);

public slots:
    void onOwnOrderSwapSuccess(swaps::SwapSuccess swap);
    void onOwnOrderSwapFailure(std::string pairId, const swaps::SwapFailure& failure);
    void onOwnOrderCompleted(const OwnOrder& ownOrder);

private slots:
    void onOwnOrderMatched(Event::ServerEvent event);
    void onConnectedToOrderbook();
    void onApiClientStateChanged(OrderbookApiClient::State newState);

private:
    using MyOrders = std::unordered_map<std::string, OwnOrder>;
    void init();
    void setState(OrderbookApiClient::State state);
    void removeOwnOrder(std::string pairId, std::string orderId);
    void removeMatchedOrder(std::string pairId, std::string orderId);
    void addOrderPortion(std::string pairId, std::string baseLocalId, OrderPortion portion);
    Promise<void> cancelOwnOrder(std::string pairId, std::string localId);
    Promise<void> cancelRemoteOwnOrder(std::string orderId);
    std::string generateId() const;

    MyOrders::const_iterator findOrderById(
        const MyOrders& where, std::string pairId, std::string orderId) const;
    MyOrders::iterator findOrderById(
        MyOrders& where, std::string pairId, std::string orderId) const;
    void eraseMyOrder(MyOrders& where, const MyOrders::const_iterator& it);
    void addMyOrder(MyOrders& where, OwnOrder ownOrder);

private:
    using OrderIdToLocalIdIndex
        = std::unordered_map<std::string, std::string>; // mapping OrderId -> LocalId
    using MyPartialOrdersIndex
        = std::unordered_map<std::string, std::string>; // mapping PartialOrderId -> BaseOrderId
    using MatchedOrders = std::unordered_map<std::string, MatchedTrade>;

    OrderbookApiClient* _apiClient{ nullptr };
    OrderbookEventDispatcher* _dispatcher{ nullptr };
    OrderbookSwapPeerPool* _peerPool;
    TradingOrdersModel* _tradingOrdersModel{ nullptr };
    OrderbookChannelRenting* _channelRenting{ nullptr };
    OrderbookRefundableFees* _refundableFees{ nullptr };
    std::unordered_map<std::string, MatchedOrders> _matchedOrders;
    std::unordered_map<std::string, MyOrders> _myorders;
    std::unordered_map<std::string, MyPartialOrdersIndex> _partialOrdersIndex;
    std::unordered_map<std::string, OrderIdToLocalIdIndex> _orderIdToLocalIdIndex;
    std::set<std::string> _subscriptions;
    OrderbookApiClient::State _state{ OrderbookApiClient::State::Disconnected };
};

std::unique_ptr<io::stakenet::orderbook::protos::BigInteger> ConvertToBigInt(int64_t value);
int64_t ConvertBigInt(io::stakenet::orderbook::protos::BigInteger bigInt);
}

#endif // ORDERBOOKCLIENT_HPP
