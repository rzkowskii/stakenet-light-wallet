#ifndef ALLORDERSDATASOURCE_HPP
#define ALLORDERSDATASOURCE_HPP

#include <Data/AbstractOrderbookDataSource.hpp>
#include <QObject>

class TradingModelBatchedProxy;

class AllOrdersDataSource : public AbstractOrderbookDataSource {
    Q_OBJECT
public:
    explicit AllOrdersDataSource(
        qobject_delete_later_unique_ptr<TradingModelBatchedProxy> ordersDataModel, QString pairId,
        QObject* parent = nullptr);

    void fetch() override;

private:
    qobject_delete_later_unique_ptr<TradingModelBatchedProxy> _ordersDataModel;
};

#endif // ALLORDERSDATASOURCE_HPP
