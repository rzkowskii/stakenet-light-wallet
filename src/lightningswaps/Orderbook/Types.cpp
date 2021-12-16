#include "Types.hpp"

//==============================================================================

orderbook::MarketOrder::MarketOrder(int64_t quantity, std::string pairId, bool isBuy)
    : quantity(quantity)
    , pairId(pairId)
    , isBuy(isBuy)
{
}

//==============================================================================

orderbook::LimitOrder::LimitOrder()
    : MarketOrder(0, {}, false)
    , price(0)
{
}

orderbook::LimitOrder::LimitOrder(orderbook::MarketOrder markerOrder, int64_t price)
    : MarketOrder(markerOrder)
    , price(price)
{
}

//==============================================================================

orderbook::OrderIdentifier::OrderIdentifier(std::string id)
    : id(id)
{
}

//==============================================================================

orderbook::Local::Local(std::string localId, int64_t hold)
    : localId(localId)
    , hold(hold)
{
}

//==============================================================================

orderbook::Remote::Remote(std::string peerPubKey)
    : peerPubKey(peerPubKey)
{
}

//==============================================================================

orderbook::Stamp::Stamp(
    orderbook::OrderIdentifier orderIdentitifer, int64_t createdAt, int64_t initialQuantity)
    : OrderIdentifier(orderIdentitifer)
    , createdAt(createdAt)
    , initialQuantity(initialQuantity)
{
}

//==============================================================================

orderbook::PeerOrder::PeerOrder(
    orderbook::LimitOrder limitOrder, orderbook::Stamp stamp, orderbook::Remote remote)
    : LimitOrder(limitOrder)
    , Stamp(stamp)
    , Remote(remote)
{
}

//==============================================================================

orderbook::OwnLimitOrder::OwnLimitOrder(
    orderbook::LimitOrder limitOrder, orderbook::Local local, std::string feePaymentHash)
    : LimitOrder(limitOrder)
    , Local(local)
    , feePaymentHash(feePaymentHash)
{
}

//==============================================================================

orderbook::OwnOrder::OwnOrder()
    : OwnLimitOrder(LimitOrder(MarketOrder(0, {}, false), 0), Local({}, 0))
    , Stamp(OrderIdentifier({}), 0, 0)
    , isOwnOrder(false)
{
}

//==============================================================================

orderbook::OwnOrder::OwnOrder(
    orderbook::OwnLimitOrder ownLimitOrder, orderbook::Stamp stamp, bool isOwnOrder)
    : OwnLimitOrder(ownLimitOrder)
    , Stamp(stamp)
    , isOwnOrder(isOwnOrder)
{
}

//==============================================================================

orderbook::Trade::Trade(std::string id, int64_t price, int64_t quantity,
    orderbook::OwnOrder ownOrder, PeerOrder peerOrder, int64_t executedOn)
    : OrderIdentifier(id)
    , price(price)
    , quantity(quantity)
    , ownOrder(ownOrder)
    , peerOrder(peerOrder)
    , executedOn(executedOn)
{
}

//==============================================================================

orderbook::OrderPortion::OrderPortion(std::string pairId, std::string orderId, int64_t quantity)
    : pairId(pairId)
    , orderId(orderId)
    , quantity(quantity)
{
}

//==============================================================================
