#ifndef ORDERBOOKDATA_HPP
#define ORDERBOOKDATA_HPP

#include <QObject>
#include <QPointer>

#if 0
#include <Orderbook/OrderbookEventDispatcher.hpp>
#include <Utils/Utils.hpp>

namespace orderbook {
class TradingOrdersModel;
class OrderDetails;
class BigInteger;
class OrderbookClient;
}

class OrderBookData : public QObject
{
    Q_OBJECT
public:
    explicit OrderBookData(QPointer<orderbook::OrderbookClient> orderbookClient, QObject *parent = nullptr);
    ~OrderBookData();

    Promise<std::vector<OrderEntry>> ordersByPairId(QString pairId);
    Promise<std::vector<OrderEntry>> ownOrders();

signals:
    void ordersChanged(std::string pairId);
    void ownOrderPlaced(OrderEntry order);
    void ownOrderMatched(std::string orderId);
    void ownOrderCanceled(std::string orderId);
    void ownOrdersChanged(std::string pairId);

private:
    QPointer<orderbook::OrderbookClient> _orderbookClient;

};
#endif
#endif // ORDERBOOKDATA_HPP
