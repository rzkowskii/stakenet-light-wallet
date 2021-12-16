#include "TradingModelBatchedDataSource.hpp"
#include <Orderbook/OrderbookClient.hpp>

using orderbook::TradingOrdersModel;

//==============================================================================

static OrderSummary ParseOrder(const TradingOrdersModel::Details& details)
{
    OrderSummary orderEntry;
    orderEntry.amount = details.amount;
    orderEntry.price = details.price;

    return orderEntry;
}

//==============================================================================

static constexpr const unsigned int UPDATE_INTERVAL_MS = 100;

//==============================================================================

TradingModelBatchedDataSource::TradingModelBatchedDataSource(
    orderbook::TradingOrdersModel* ordersModel, QObject* parent)
    : QObject(parent)
    , _ordersDataModel(ordersModel)
{
    init();
}

//==============================================================================

Promise<AbstractOrderbookDataSource::Orders> TradingModelBatchedDataSource::fetch(
    QString pairId, AbstractOrderbookDataSource::OrdersType type)
{
    return Promise<AbstractOrderbookDataSource::Orders>(
        [dataModel = _ordersDataModel, pairId, type](const auto& resolve, const auto&) {
            QMetaObject::invokeMethod(dataModel, [=] {
                AbstractOrderbookDataSource::Orders result;

                const auto isBuy = type == AbstractOrderbookDataSource::OrdersType::Buy;

                auto transform = [&result](auto& orders) {
                    result.reserve(orders.size());
                    std::transform(std::begin(orders), std::end(orders), std::back_inserter(result),
                        [](const auto& it) {
                            return OrderSummary(it.second.price, it.second.amount);
                        });
                };

                if (isBuy) {
                    transform(dataModel->bidOrders(pairId.toStdString()));
                } else {
                    transform(dataModel->askOrders(pairId.toStdString()));
                }

                resolve(result);
            });
        });
}

//==============================================================================

void TradingModelBatchedDataSource::onOrderAdded(
    std::string pairId, OrderSummary entry, Enums::OrderSide side)
{
    _pendingOrderQueue[pairId][side == Enums::OrderSide::Sell][entry.price] += entry.amount;
}

//==============================================================================

void TradingModelBatchedDataSource::onOrderRemoved(
    std::string pairId, OrderSummary entry, Enums::OrderSide side)
{
    _pendingOrderQueue[pairId][side == Enums::OrderSide::Sell][entry.price] -= entry.amount;
}

//==============================================================================

void TradingModelBatchedDataSource::init()
{
    static std::once_flag once;
    std::call_once(once, [] {
        qRegisterMetaType<AbstractOrderbookDataSource::UpdatesBatch>(
            "AbstractOrderbookDataSource::UpdatesBatch");
        qRegisterMetaType<AbstractOrderbookDataSource::Orders>(
            "AbstractOrderbookDataSource::Orders");
    });
    connect(_ordersDataModel, &TradingOrdersModel::ordersChanged, this,
        [this](auto pairId) { this->ordersChanged(QString::fromStdString(pairId)); });

    connect(_ordersDataModel, &TradingOrdersModel::orderAdded, this,
        [this](auto pairId, auto details, auto isAsk) {
            this->onOrderAdded(pairId, ParseOrder(details),
                isAsk ? Enums::OrderSide::Sell : Enums::OrderSide::Buy);
        });
    connect(_ordersDataModel, &TradingOrdersModel::orderRemoved, this,
        [this](auto pairId, auto details, auto isAsk) {
            this->onOrderRemoved(pairId, ParseOrder(details),
                isAsk ? Enums::OrderSide::Sell : Enums::OrderSide::Buy);
        });

    startTimer(UPDATE_INTERVAL_MS);
}

//==============================================================================

void TradingModelBatchedDataSource::timerEvent(QTimerEvent* event)
{
    Q_UNUSED(event);

    decltype(_pendingOrderQueue) pendingOrderQueue;
    pendingOrderQueue.swap(_pendingOrderQueue);

    auto transform = [](const auto& orders, auto& where) {
        std::transform(
            std::begin(orders), std::end(orders), std::back_inserter(where), [](const auto& it) {
                return it.second > 0
                    ? AbstractOrderbookDataSource::UpdateEvent{ AbstractOrderbookDataSource::
                              OrderAddedEvent{ OrderSummary(it.first, it.second) } }
                    : AbstractOrderbookDataSource::UpdateEvent{
                          AbstractOrderbookDataSource::OrderRemovedEvent{
                              OrderSummary(it.first, std::abs(it.second)) }
                      };
            });
    };

    for (auto&& pendingOrdersIt : std::move(pendingOrderQueue)) {
        std::array<AbstractOrderbookDataSource::UpdatesBatch, 2> batch;

        transform(pendingOrdersIt.second.at(0), batch.at(0));
        transform(pendingOrdersIt.second.at(1), batch.at(1));

        if (!batch.at(0).empty()) {
            buyOrdersUpdated(QString::fromStdString(pendingOrdersIt.first), batch.at(0));
        }
        if (!batch.at(1).empty()) {
            sellOrdersUpdated(QString::fromStdString(pendingOrdersIt.first), batch.at(1));
        }
    }
}

//==============================================================================
