#ifndef TRADINGMODELBATCHEDDATASOURCE_HPP
#define TRADINGMODELBATCHEDDATASOURCE_HPP

#include <QObject>
#include <unordered_map>

#include <Data/AbstractOrderbookDataSource.hpp>
#include <Orderbook/TradingOrdersModel.hpp>

class TradingModelBatchedDataSource : public QObject {
    Q_OBJECT
public:
    explicit TradingModelBatchedDataSource(
        orderbook::TradingOrdersModel* ordersModel, QObject* parent = nullptr);

    Promise<AbstractOrderbookDataSource::Orders> fetch(
        QString pairId, AbstractOrderbookDataSource::OrdersType type);

signals:
    void ordersChanged(QString pairId);
    void buyOrdersUpdated(QString pairId, const AbstractOrderbookDataSource::UpdatesBatch& updates);
    void sellOrdersUpdated(
        QString pairId, const AbstractOrderbookDataSource::UpdatesBatch& updates);

protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    void onOrderAdded(std::string pairId, OrderSummary entry, Enums::OrderSide side);
    void onOrderRemoved(std::string pairId, OrderSummary entry, Enums::OrderSide side);

private:
    void init();

private:
    /* map of price to amount */
    using PendingOrders = std::unordered_map<Balance, Balance>;
    orderbook::TradingOrdersModel* _ordersDataModel{ nullptr };
    /* { pairId, { buy pending orders map, sell pending orders map } */
    std::unordered_map<std::string, std::array<PendingOrders, 2>> _pendingOrderQueue;
};

#endif // TRADINGMODELBATCHEDDATASOURCE_HPP
