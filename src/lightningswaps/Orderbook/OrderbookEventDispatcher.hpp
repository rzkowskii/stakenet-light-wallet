#ifndef ORDERBOOKEVENTDISPATCHER_HPP
#define ORDERBOOKEVENTDISPATCHER_HPP

#include <Orderbook/OrderbookApiClient.hpp>
#include <QObject>
#include <functional>
#include <unordered_map>

namespace orderbook {

class OrderbookEventDispatcher : public QObject {
    Q_OBJECT
public:
    explicit OrderbookEventDispatcher(OrderbookApiClient& connection, QObject* parent = nullptr);

    using OnEventHandler = std::function<void(const Event::ServerEvent&)>;
    void addEventHandler(Event::ServerEvent::ValueCase event, OnEventHandler handler);

private slots:
    void onEventReceived(Event::ServerEvent event);

private:
    std::unordered_map<unsigned int, std::vector<OnEventHandler>> _handlers;
};
}

#endif // ORDERBOOKEVENTDISPATCHER_HPP
