#include "TradingModelBatchedProxy.hpp"
#include <Data/TradingModelBatchedDataSource.hpp>
#include <Tools/Common.hpp>

//==============================================================================

TradingModelBatchedProxy::TradingModelBatchedProxy(TradingModelBatchedDataSource* batchedDataSource,
    QString pairId, AbstractOrderbookDataSource::OrdersType type, QObject* parent)
    : QObject(parent)
    , _dataSource(batchedDataSource)
    , _type(type)
    , _pairId(pairId)
{
    connect(_dataSource, &TradingModelBatchedDataSource::ordersChanged, this,
        &TradingModelBatchedProxy::onOrdersChanged);

    switch (_type) {
    case AbstractOrderbookDataSource::OrdersType::Buy:
        connect(_dataSource, &TradingModelBatchedDataSource::buyOrdersUpdated, this,
            &TradingModelBatchedProxy::onOrdersUpdated);
        break;
    case AbstractOrderbookDataSource::OrdersType::Sell:
        connect(_dataSource, &TradingModelBatchedDataSource::sellOrdersUpdated, this,
            &TradingModelBatchedProxy::onOrdersUpdated);
        break;
    }
}

//==============================================================================

AbstractOrderbookDataSource::OrdersType TradingModelBatchedProxy::type() const
{
    return _type;
}

//==============================================================================

void TradingModelBatchedProxy::fetch()
{
    _dataSource->fetch(_pairId, _type)
        .then([this](const AbstractOrderbookDataSource::Orders& orders) {
            this->onOrdersFetched(orders);
        });
}

//==============================================================================

void TradingModelBatchedProxy::onOrdersFetched(AbstractOrderbookDataSource::Orders orders)
{
    std::sort(std::begin(orders), std::end(orders),
        [this](const OrderSummary& lhs, const OrderSummary& rhs) {
            return _type == AbstractOrderbookDataSource::OrdersType::Buy ? lhs.price > rhs.price
                                                                         : lhs.price < rhs.price;
        });

    ordersChanged(orders);
}

//==============================================================================

void TradingModelBatchedProxy::onOrdersUpdated(
    QString pairId, const AbstractOrderbookDataSource::UpdatesBatch& updates)
{
    if (_pairId != pairId) {
        return;
    }

    ordersUpdated(updates);
}

//==============================================================================

void TradingModelBatchedProxy::onOrdersChanged(QString pairId)
{
    if (_pairId != pairId) {
        return;
    }

    fetch();
}

//==============================================================================
