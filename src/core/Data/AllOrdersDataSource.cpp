#include "AllOrdersDataSource.hpp"
#include <Data/TradingModelBatchedProxy.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/TradingOrdersModel.hpp>

using orderbook::TradingOrdersModel;

//==============================================================================

AllOrdersDataSource::AllOrdersDataSource(
    qobject_delete_later_unique_ptr<TradingModelBatchedProxy> ordersDataModel, QString pairId,
    QObject* parent)
    : AbstractOrderbookDataSource(ordersDataModel->type(), pairId, parent)
    , _ordersDataModel(std::move(ordersDataModel))
{
    connect(_ordersDataModel.get(), &TradingModelBatchedProxy::ordersChanged, this,
        &AllOrdersDataSource::setOrders);

    connect(_ordersDataModel.get(), &TradingModelBatchedProxy::ordersUpdated, this,
        [this](const auto& batch) {
            std::vector<OrderSummary> ordersAdded;
            std::vector<OrderSummary> ordersRemoved;

            for (auto&& update : batch) {
                if (auto added
                    = boost::get<AbstractOrderbookDataSource::OrderAddedEvent>(&update)) {
                    ordersAdded.emplace_back(added->order);
                } else {
                    auto removed
                        = boost::get<AbstractOrderbookDataSource::OrderRemovedEvent>(&update);
                    ordersRemoved.emplace_back(removed->order);
                }
            }

            this->updateOrders(ordersAdded, ordersRemoved);
        });
}

//==============================================================================

void AllOrdersDataSource::fetch()
{
    if (!isOrdersSet()) {
        QMetaObject::invokeMethod(_ordersDataModel.get(), [dataModel = _ordersDataModel.get()] {
            if (dataModel) {
                dataModel->fetch();
            }
        });
    }
}

//==============================================================================
