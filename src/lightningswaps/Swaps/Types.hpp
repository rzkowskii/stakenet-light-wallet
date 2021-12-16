#ifndef SWAPS_TYPES_HPP
#define SWAPS_TYPES_HPP

#include <QException>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <string>
#include <map>

namespace swaps {

using u256 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256,
    boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;

static std::map<std::string, int64_t> UNITS_PER_CURRENCY = { { "BTC", 8 }, { "LTC", 8 },
                                                             { "ETH", 18 }, { "WETH", 18 }, { "DAI", 18 }, { "TTT", 18 }, { "XSN", 8 }, { "USDT", 6 }, { "rETH", 18 }, { "USDC", 6}};

enum class ClientType { Lnd, Connext };

enum class SwapRole { Taker, Maker };

enum class OrderType { Limit, Market };

enum class SwapState { Active, Error, Completed };

enum class SwapPhase {
    /** The swap deal has been created locally. */
    SwapCreated,
    /** We've made a request to a peer to accept this swap. */
    SwapRequested,
    /** The terms of the swap have been agreed to, and we will attempt to execute it. */
    SwapAccepted,
    InvoiceExchange,
    /**
     * We have made a request to the swap client to send payment according to the agreed terms.
     * The payment (and swap) could still fail due to no route with sufficient capacity, lack of
     * cooperation from the receiver or any intermediary node along the route, or an unexpected
     * error from the swap client.
     */
    SendingPayment,
    /**
     * We have received the agreed amount of the swap and released the preimage to the
     * receiving swap client so it can accept payment.
     */
    PaymentReceived,
    /** A special state to notify maker that taker has completed his part. We need this state to
     *  know when exactly SwapCompleted was broadcasted through network channel. Without this state
     *  we don't know when to finalize maker swap.
     */
    SwapTakerCompleted,
    /** The swap has been formally completed and both sides have confirmed they've received payment.
     */
    SwapCompleted
};

enum class SwapFailureReason {
    /** Could not find the order specified by a swap request. */
    OrderNotFound,
    /** The order specified by a swap request is on hold for a different ongoing swap. */
    OrderOnHold,
    /** The swap request contained invalid data. */
    InvalidSwapRequest,
    /** We are not connected to both swap clients, or we are missing pub key identifiers for the
       peer's nodes. */
    SwapClientNotSetup,
    /** Could not find a route to complete the swap. */
    NoRouteFound,
    /** A swap client call failed for an unexpected reason. */
    UnexpectedClientError,
    /** Received a swap packet from the peer with invalid data. */
    InvalidSwapPacketReceived,
    /** The call to send payment failed. */
    SendPaymentFailure,
    /** The swap resolver request was invalid. */
    InvalidResolveRequest,
    /** The swap request attempts to reuse a payment hash. */
    PaymentHashReuse,
    /** The swap timed out while we were waiting for it to complete execution. */
    SwapTimedOut,
    /** The deal timed out while we were waiting for the peer to respond to our swap request. */
    DealTimedOut,
    /** The swap failed due to an unrecognized error. */
    UnknownError,
    /** The swap failed due to an error or unexpected behavior on behalf of the remote peer. */
    RemoteError,
    /** Tried to execute swap to ourself */
    SwapToYourself,
};

static std::map<std::string, ClientType> CLIENT_TYPE_PER_CURRENCY = {
    { "BTC", ClientType::Lnd },
    { "LTC", ClientType::Lnd},
    { "XSN", ClientType::Lnd },
    { "ETH", ClientType::Connext },
    { "WETH", ClientType::Connext },
    { "USDT", ClientType::Connext },
    { "USDC", ClientType::Connext },
    { "rETH", ClientType::Connext }};

struct SwapSuccess {
    /** The result of a successful swap. */
    std::string orderId;
    std::string localId;
    std::string pairId;
    std::string rHash;
    std::string peerPubKey;
    int64_t price;
    std::string rPreimage;
    /** Our role in the swap. */
    SwapRole role;
    /** Type of the order. */
    OrderType type;
    /** The amount received denominated in satoshis. */
    int64_t amountReceived;
    /** The amount sent denominated in satoshis. */
    int64_t amountSent;
    /** The ticker symbol of the currency received. */
    std::string currencyReceived;
    /** The ticker symbol of the currency sent. */
    std::string currencySent;
    /** The quantity that was swapped. */
    int64_t quantity;
};

struct SwapFailure {
    /** The global UUID for the order that failed the swap. */
    std::string orderId;
    /** The local id of the own order involved in the swap. */
    std::string localId;
    /** The trading pair that the swap is for. */
    std::string pairId;
    /** The order quantity that was attempted to be swapped. */
    int64_t quantity;
    /** The reason why the swap failed. */
    SwapFailureReason failureReason;
    /** The message why the swap failed. */
    std::string failureMessage;
    /** Our role in the swap. */
    SwapRole role;
};

struct SwapDeal {
    /** Our role in the swap. */
    SwapRole role;
    /** The most updated deal phase */
    SwapPhase phase;
    /**
     * The most updated deal state. State works together with phase to indicate where the
     * deal is in its life cycle and if the deal is active, errored, or completed.
     */
    SwapState state;
    /** The reason for being in the current state. */
    boost::optional<std::string> errorMessage;
    boost::optional<SwapFailureReason> failureReason;
    /** The xud node pub key of the counterparty to this swap deal. */
    std::string peerPubKey;
    /** The trading pair for the swap. The pairId together with the orderId are needed to find the
     * maker order in the order book. */
    std::string orderId;
    /** Type of the order. */
    OrderType orderType;
    /** Whether the maker order is a buy order. */
    bool isBuy;
    /** The local id of the own order involved in the swap. */
    std::string localId;
    /** The quantity of the order to execute as proposed by the taker. */
    int64_t proposedQuantity;
    /** The quantity of the order to execute as accepted by the maker. */
    boost::optional<int64_t> quantity;
    /** The trading pair for the swap. The pairId together with the orderId are needed to find the
     * maker order in the order book. */
    std::string pairId;
    /** The amount the taker is expecting to receive denominated in satoshis. */
    int64_t takerAmount;
    /** The number of the smallest base units of the currency (like satoshis or wei) the maker is
     * expecting to receive. */
    u256 takerUnits;
    /** The currency the taker is expecting to receive. */
    std::string takerCurrency;
    /** Taker's pubkey on the taker currency's network. */
    boost::optional<std::string> takerPubKey;
    /** Maker's pubkey on the maker currency's network. */
    boost::optional<std::string> makerPubKey;
    /** The CLTV delta from the current height that should be used to set the timelock for the final
     * hop when sending to taker. */
    uint32_t takerCltvDelta;
    /** The amount the maker is expecting to receive denominated in satoshis. */
    int64_t makerAmount;
    /** The number of the smallest base units of the currency (like satoshis or wei) the maker is
     * expecting to receive. */
    u256 makerUnits;
    /** The currency the maker is expecting to receive. */
    std::string makerCurrency;
    /** The CLTV delta from the current height that should be used to set the timelock for the final
     * hop when sending to maker. */
    boost::optional<uint32_t> makerCltvDelta;
    /** The price of the order that's being executed. */
    int64_t price;
    /** The hex-encoded hash of the preimage. */
    std::string rHash;
    /** The hex-encoded preimage. */
    boost::optional<std::string> rPreimage;
    /** The maximum time lock from the maker to the taker in blocks. */
    boost::optional<uint32_t> takerMaxTimeLock;
    /** The identifier for the payment channel network node we should pay to complete the swap.  */
    boost::optional<std::string> destination;
    /** The time when we created this swap deal locally. */
    int64_t createTime;
    /** The time when we began executing the swap by sending payment. */
    boost::optional<int64_t> executeTime;
    /** The time when the swap either completed successfully or failed. */
    boost::optional<int64_t> completeTime;

    boost::optional<std::string> paymentRequest;
};

/** An order without a price that is intended to match against any available orders on the opposite
 * side of the book for its trading pair. */
struct MarketOrder {
    /** The number of currently satoshis (or equivalent) for the order. */
    int64_t quantity;
    /** A trading pair symbol with the base currency first followed by a '/' separator and the quote
     * currency */
    std::string pairId;
    /** Whether the order is a buy (if `true`) or a sell (if `false`). */
    bool isBuy;
    /** Whether the order type is a limit or a market . */
    OrderType type;
};

/** A limit order with a specified price that will enter the order book if it is not immediately
 * matched. */
struct LimitOrder : MarketOrder {
    /** The price for the order expressed in units of the quote currency. */
    int64_t price;
};

/** Properties that apply only to orders placed by the local xud. */
struct Local {
    /** A local identifier for the order. */
    std::string localId;
    /** The amount of an order that is on hold pending swap execution. */
    int64_t hold;
};

/** Properties that apply only to orders placed by remote peers. */
struct Remote {
    /** The nodePubKey of the node which created this order. */
    std::string peerPubKey;
};

struct OrderIdentifier {
    /** The global identifier for this order on the XU network. */
    std::string id;
};

/** Properties that uniquely identify an order and make it ready to enter the order book. */
struct Stamp : OrderIdentifier {
    /** Epoch timestamp when this order was created locally. */
    int64_t createdAt;
    /** The number of satoshis (or equivalent) initially available for the order, before any actions
     * such as trades reduced the available quantity. */
    int64_t initialQuantity;
};

struct PeerOrder : LimitOrder, Stamp, Remote {
};

struct OwnLimitOrder : LimitOrder, Local {
};

struct OwnOrder : OwnLimitOrder, Stamp {
};

struct OrderToAccept {
    std::string localId;
    int64_t price;
    int64_t quantity;
    bool isBuy;
};

class SwapException : public std::runtime_error {
public:
    SwapException(SwapFailureReason reason, std::string message = {})
        : std::runtime_error(message)
        , reason(reason)
    {
    }

    SwapFailureReason reason;
};

struct ResolveRequest {
    /** The amount of the incoming payment pending resolution, in the smallest units supported by
     * the token. */
    int64_t amount;
    std::string rHash;
    /** The number of blocks before the incoming payment expires. */
    uint32_t expiration;
    /** Public key of initiator */
    std::string initiatorIdentifier;
    /** Public key of responder */
    std::string responderIdentifier;

    std::string transferId;
    std::string channelAddress;
};

struct ResolvedTransferResponse {
    /** rHash of settled transfer */
    std::string lockHash;
    /** rPreimage of settled transfer */
    std::string rPreimage;
};

struct MakerTakerAmounts {
    struct Amount {
        std::string currency;
        int64_t amount; // amount in satoshis (10^8)
        u256 units; // amount in smallest denomination(wei, sats, etc)
    } maker, taker;

    /**
     * Calculates the incoming and outgoing currencies and amounts of subunits/satoshis for an order
     * if it is swapped.
     * @param quantity The quantity of the order in base currency
     * @param price The price of the order in quote currency
     * @param isBuy Whether the order is a buy
     * @returns An object with the calculated incoming and outgoing values. The quote currency
     * amount is returned as zero if the price is 0 or infinity, indicating a market order.
     */
    static MakerTakerAmounts CalculateMakerTakerAmounts(
        int64_t quantity, int64_t price, bool isBuy, std::string pairId);

    /**
     * Converts given total to order quantity in base currency
     * @param quantity The total quantity of the order in quote currency
     * @param price The price of the order in quote currency
     * @param pairId id of the swap pair
     * @returns amount of the order in base currency
     * */
    static int64_t CalculateOrderAmount(int64_t total, int64_t price, std::string pairId);

    std::string toString() const;
};
}

#endif // SWAPS_TYPES_HPP
