#ifndef ORDERBOOK_TYPES_HPP
#define ORDERBOOK_TYPES_HPP

#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace orderbook {
/** Status for placing own orders */
enum class OwnOrderStatus {
    Pending,
    Confirmed
};

enum class OwnOrderType {
    Limit,
    Market
};

/** An order without a price that is intended to match against any available orders on the opposite
 * side of the book for its trading pair. */
struct MarketOrder {
    /** The number of currently satoshis (or equivalent) for the order. For buy order in quote
     * currency, for sell order in base currency. */
    int64_t quantity;
    /** A trading pair symbol with the base currency first followed by a '/' separator and the quote
     * currency */
    std::string pairId;
    /** Whether the order is a buy (if `true`) or a sell (if `false`). */
    bool isBuy;

    explicit MarketOrder(int64_t quantity, std::string pairId, bool isBuy);
};

/** A limit order with a specified price that will enter the order book if it is not immediately
 * matched. */
struct LimitOrder : MarketOrder {
    /** The price for the order expressed in units of the quote currency. */
    int64_t price;

    LimitOrder();
    explicit LimitOrder(MarketOrder markerOrder, int64_t price);
};

/** Properties that can be used to uniquely identify and fetch an order within an order book. */
struct OrderIdentifier {
    /** The global identifier for this order on the XU network. */
    std::string id;

    explicit OrderIdentifier(std::string id);
};

/** Properties that apply only to orders placed by the local xud. */
struct Local {
    /** A local identifier for the order. */
    std::string localId;
    /** The amount of an order that is on hold pending swap execution. */
    int64_t hold;

    explicit Local(std::string localId, int64_t hold);
};

/** Properties that apply only to orders placed by remote peers. */
struct Remote {
    /** The nodePubKey of the node which created this order. */
    std::string peerPubKey;

    explicit Remote(std::string peerPubKey);
};

/** Properties that uniquely identify an order and make it ready to enter the order book. */
struct Stamp : OrderIdentifier {

    /** Epoch timestamp when this order was created locally. */
    int64_t createdAt;
    /** The number of satoshis (or equivalent) initially available for the order, before any actions
     * such as trades reduced the available quantity. */
    int64_t initialQuantity;

    explicit Stamp(OrderIdentifier orderIdentitifer, int64_t createdAt, int64_t initialQuantity);
};

struct PeerOrder : LimitOrder, Stamp, Remote {
    explicit PeerOrder(LimitOrder limitOrder, Stamp stamp, Remote remote);
};

struct OwnLimitOrder : LimitOrder, Local {
    /** rHash of payment that was used to pay fee, needs to be passed when there is fee required by
     * orderbook */
    std::string feePaymentHash;

    explicit OwnLimitOrder(LimitOrder limitOrder, Local local, std::string feePaymentHash = {});
};

struct OrderPortion {
    std::string pairId;
    std::string orderId;
    int64_t quantity;

    explicit OrderPortion(std::string pairId, std::string orderId, int64_t quantity);
};

struct OwnOrder : OwnLimitOrder, Stamp {
    std::vector<OrderPortion> openPortions;
    std::vector<OrderPortion> closedPortions;
    OwnOrderStatus orderStatus {OwnOrderStatus::Pending};
    OwnOrderType orderType {OwnOrderType::Limit};
    bool isOwnOrder;

    OwnOrder(); // TODO(yuraolex): needed for qRegisterMetaType, fix later
    explicit OwnOrder(OwnLimitOrder ownLimitOrder, Stamp stamp, bool isOwnOrder);
};

struct Trade : OrderIdentifier {
    int64_t price;
    int64_t quantity;
    OwnOrder ownOrder;
    PeerOrder peerOrder;
    int64_t executedOn;

    explicit Trade(std::string id, int64_t price, int64_t quantity, OwnOrder ownOrder,
        PeerOrder peerOrder, int64_t executedOn);
};

struct TradingPair {
    using BalanceInterval = std::pair<int64_t, int64_t>;

    std::string pairId;
    BalanceInterval buyFundsInterval;
    BalanceInterval buyPriceInterval;
    BalanceInterval sellFundsInterval;
    BalanceInterval sellPriceInterval;
    double buyFeePercent;
    double sellFeePercent;
};

using PlaceOrderOutcome = boost::variant<OwnOrder, OrderPortion, Trade>;
}

#endif // ORDERBOOK_TYPES_HPP
