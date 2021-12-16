#include "OrderbookEventDispatcher.hpp"
#include <Utils/Logging.hpp>

namespace orderbook {

//==============================================================================

OrderbookEventDispatcher::OrderbookEventDispatcher(OrderbookApiClient& connection, QObject* parent)
    : QObject(parent)
{
    connect(&connection, &OrderbookApiClient::notificationReceived, this,
        &OrderbookEventDispatcher::onEventReceived);
}

//==============================================================================

void OrderbookEventDispatcher::addEventHandler(
    Event::ServerEvent::ValueCase event, OrderbookEventDispatcher::OnEventHandler handler)
{
    _handlers[event].emplace_back(handler);
}

//==============================================================================

void OrderbookEventDispatcher::onEventReceived(Event::ServerEvent event)
{
    auto it = _handlers.find(event.value_case());

    if (it != std::end(_handlers)) {
        for (const auto& handler : it->second) {
            handler(event);
        }
    }
}

//==============================================================================
}
