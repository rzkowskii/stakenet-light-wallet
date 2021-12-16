#include "SwapsServiceNotifications.hpp"
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/TradingOrdersModel.hpp>
#include <Swaps/Types.hpp>

//==============================================================================

template <class From, class Event> static void SendEventToStream(const From& from, Event&& event)
{
    for (auto& stream : from) {
        stream->send(event);
    }
}

//==============================================================================

static std::unique_ptr<lssdrpc::BigInteger> ConvertToBigInt(int64_t value)
{
    std::unique_ptr<lssdrpc::BigInteger> result(new lssdrpc::BigInteger);
    result->set_value(std::to_string(value));
    return result;
}

//==============================================================================

template <class T>
void removeDisconnectedSubscriptions(
    std::vector<SwapsServiceNotifications::StreamingChanPtr<T>>& where)
{
    where.erase(
        std::remove_if(where.begin(), where.end(),
            [](const SwapsServiceNotifications::StreamingChanPtr<T>& sub) { return sub.unique(); }),
        where.end());
}

//==============================================================================

SwapsServiceNotifications::SwapsServiceNotifications(
    swaps::SwapService& swapService, QObject* parent)
    : QObject(parent)
{
    init(swapService);
}

//==============================================================================

void SwapsServiceNotifications::addSubscription(StreamingChanPtr<lssdrpc::SwapResult> swapSuccess)
{
    _swapsSubscriptions.emplace_back(swapSuccess);
}

//==============================================================================

void SwapsServiceNotifications::addSubscription(StreamingChanPtr<lssdrpc::OrderUpdate> orderUpdate)
{
    _ordersSubscriptions.emplace_back(orderUpdate);
}

//==============================================================================

void SwapsServiceNotifications::addSubscription(
    StreamingChanPtr<lssdrpc::OwnOrderUpdate> ownOrderUpdate)
{
    _ownOrdersSubscriptions.emplace_back(ownOrderUpdate);
}

//==============================================================================

void SwapsServiceNotifications::addSubscription(
    StreamingChanPtr<lssdrpc::OrderbookState> orderbookStateUpdate)
{
    _orderbookStateSubscriptions.emplace_back(orderbookStateUpdate);
}

//==============================================================================

void SwapsServiceNotifications::onSwapSuccessReceived(swaps::SwapSuccess success)
{
    removeDisconnectedSubscriptions(_swapsSubscriptions);

    auto swap = new lssdrpc::SwapSuccess;
    swap->set_orderid(success.orderId);
    swap->set_pairid(success.pairId);
    swap->set_allocated_funds(ConvertToBigInt(success.quantity).release());
    swap->set_rhash(success.rHash);
    swap->set_allocated_amountreceived(ConvertToBigInt(success.amountReceived).release());
    swap->set_allocated_amountsent(ConvertToBigInt(success.amountSent).release());
    swap->set_currencyreceived(success.currencyReceived);
    swap->set_currencysent(success.currencySent);
    swap->set_rpreimage(success.rPreimage);
    swap->set_allocated_price(ConvertToBigInt(success.price).release());
    swap->set_role([role = success.role] {
        switch (role) {
        case swaps::SwapRole::Maker:
            return lssdrpc::SwapSuccess::Role::SwapSuccess_Role_MAKER;
        case swaps::SwapRole::Taker:
            return lssdrpc::SwapSuccess::Role::SwapSuccess_Role_TAKER;
        }
    }());

    lssdrpc::SwapResult result;
    result.set_allocated_success(swap);

    SendEventToStream(_swapsSubscriptions, result);
}

//==============================================================================

void SwapsServiceNotifications::onSwapFailureReceived(swaps::SwapFailure swap)
{
    removeDisconnectedSubscriptions(_swapsSubscriptions);

    auto failedSwap = new lssdrpc::SwapFailure;
    failedSwap->set_pairid(swap.pairId);
    failedSwap->set_orderid(swap.orderId);
    failedSwap->set_allocated_funds(ConvertToBigInt(swap.quantity).release());
    failedSwap->set_failurereason(swap.failureMessage);

    lssdrpc::SwapResult result;
    result.set_allocated_failure(failedSwap);

    SendEventToStream(_swapsSubscriptions, result);
}

//==============================================================================

void SwapsServiceNotifications::init(swaps::SwapService& swapService)
{
    connectSwaps(swapService);
    connectOrders(swapService.orderBookClient());
    connectOwnOrders(swapService.orderBookClient());
    connect(swapService.orderBookClient(), &orderbook::OrderbookClient::stateChanged, this, [this](auto state) {
        lssdrpc::OrderbookState lssdState;
        lssdState.set_state([](auto state) {
            switch(state) {
                case orderbook::OrderbookApiClient::State::Connected: return lssdrpc::OrderbookState_State::OrderbookState_State_CONNECTED;
                case orderbook::OrderbookApiClient::State::Disconnected: return lssdrpc::OrderbookState_State::OrderbookState_State_DISCONNECTED;
            };

            return lssdrpc::OrderbookState_State::OrderbookState_State_UNKNOWN;
        }(state));

        SendEventToStream(_orderbookStateSubscriptions, lssdState);
    });
}

//==============================================================================

void SwapsServiceNotifications::connectSwaps(swaps::SwapService& swapService)
{
    connect(&swapService, &swaps::SwapService::swapExecuted, this,
        &SwapsServiceNotifications::onSwapSuccessReceived);

    connect(&swapService, &swaps::SwapService::swapFailed, this,
        &SwapsServiceNotifications::onSwapFailureReceived);
}

//==============================================================================

void SwapsServiceNotifications::connectOrders(orderbook::OrderbookClient* orderbookClient)
{
    using orderbook::OrderbookClient;
    using orderbook::TradingOrdersModel;

    connect(
        orderbookClient, &OrderbookClient::orderAdded, this, [this](orderbook::LimitOrder order) {
            removeDisconnectedSubscriptions(_ordersSubscriptions);
            lssdrpc::OrderUpdate update;
            auto& added = *update.mutable_orderadded();
            added.set_pairid(order.pairId);
            added.set_allocated_funds(ConvertToBigInt(order.quantity).release());
            added.set_allocated_price(ConvertToBigInt(order.price).release());
            SendEventToStream(_ordersSubscriptions, update);
        });

    connect(
        orderbookClient, &OrderbookClient::orderRemoved, this, [this](orderbook::LimitOrder order) {
            removeDisconnectedSubscriptions(_ordersSubscriptions);
            lssdrpc::OrderUpdate update;
            auto& removed = *update.mutable_orderremoval();
            removed.set_pairid(order.pairId);
            removed.set_allocated_funds(ConvertToBigInt(order.quantity).release());
            removed.set_allocated_price(ConvertToBigInt(order.price).release());
            SendEventToStream(_ordersSubscriptions, update);
        });
}

//==============================================================================

void SwapsServiceNotifications::connectOwnOrders(orderbook::OrderbookClient* orderbookClient)
{
    using orderbook::OrderbookClient;
    connect(orderbookClient, &OrderbookClient::ownOrderChanged, this,
        [this](orderbook::OwnOrder order) {
            removeDisconnectedSubscriptions(_ownOrdersSubscriptions);
            lssdrpc::OwnOrderUpdate update;
            FillOrderHelper(*update.mutable_orderchanged(), order);
            SendEventToStream(_ownOrdersSubscriptions, update);
        });

    connect(
        orderbookClient, &OrderbookClient::ownOrderPlaced, this, [this](orderbook::OwnOrder order) {
            removeDisconnectedSubscriptions(_ownOrdersSubscriptions);
            lssdrpc::OwnOrderUpdate update;
            FillOrderHelper(*update.mutable_orderadded(), order);
            SendEventToStream(_ownOrdersSubscriptions, update);
        });

    connect(
        orderbookClient, &OrderbookClient::ownOrderCompleted, this, [this](std::string orderId) {
            removeDisconnectedSubscriptions(_ownOrdersSubscriptions);
            lssdrpc::OwnOrderUpdate update;
            update.set_ordercompleted(orderId);
            SendEventToStream(_ownOrdersSubscriptions, update);
        });
}

//==============================================================================

void FillOrderHelper(lssdrpc::Order& orderToFill, const orderbook::OwnOrder& order)
{
    orderToFill.set_pairid(order.pairId);
    orderToFill.set_side(order.isBuy ? lssdrpc::OrderSide::buy : lssdrpc::OrderSide::sell);
    orderToFill.set_orderid(order.id);
    orderToFill.set_isownorder(order.isOwnOrder);
    orderToFill.set_allocated_funds(ConvertToBigInt(order.quantity).release());
    orderToFill.set_allocated_price(ConvertToBigInt(order.price).release());

    for (auto&& portion : order.openPortions) {
        auto open = orderToFill.add_open();
        open->set_orderid(portion.orderId);
        open->set_allocated_amount(ConvertToBigInt(portion.quantity).release());
    }

    for (auto&& portion : order.closedPortions) {
        auto closed = orderToFill.add_closed();
        closed->set_orderid(portion.orderId);
        closed->set_allocated_amount(ConvertToBigInt(portion.quantity).release());
    }
}

//==============================================================================
