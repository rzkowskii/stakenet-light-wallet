#ifndef TRADINGMODELBATCHEDPROXY_HPP
#define TRADINGMODELBATCHEDPROXY_HPP

#include <Data/AbstractOrderbookDataSource.hpp>
#include <QObject>
#include <QPointer>

class TradingModelBatchedDataSource;

class TradingModelBatchedProxy : public QObject {
    Q_OBJECT
public:
    explicit TradingModelBatchedProxy(TradingModelBatchedDataSource* batchedDataSource,
        QString pairId, AbstractOrderbookDataSource::OrdersType type, QObject* parent = nullptr);

    AbstractOrderbookDataSource::OrdersType type() const;
    void fetch();

signals:
    void ordersChanged(const AbstractOrderbookDataSource::Orders& orders);
    void ordersUpdated(const AbstractOrderbookDataSource::UpdatesBatch& updates);

private slots:
    void onOrdersFetched(AbstractOrderbookDataSource::Orders orders);
    void onOrdersUpdated(QString pairId, const AbstractOrderbookDataSource::UpdatesBatch& updates);
    void onOrdersChanged(QString pairId);

private:
    QPointer<TradingModelBatchedDataSource> _dataSource;
    AbstractOrderbookDataSource::OrdersType _type;
    QString _pairId;
};

#endif // TRADINGMODELBATCHEDPROXY_HPP
