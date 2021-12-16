#include "AbstractOrderbookDataSource.hpp"

#include <cmath>

//==============================================================================

AbstractOrderbookDataSource::AbstractOrderbookDataSource(
    OrdersType type, QString pairId, QObject* parent)
    : QObject(parent)
    , _pairId(pairId.toStdString())
    , _type(type)
{
}

//==============================================================================

QString AbstractOrderbookDataSource::pairId() const
{
    return QString::fromStdString(_pairId);
}

//==============================================================================

const AbstractOrderbookDataSource::Orders& AbstractOrderbookDataSource::orders() const
{
    return _orders;
}

//==============================================================================

int64_t AbstractOrderbookDataSource::sumAmounts() const
{
    return _sumAmounts;
}

//==============================================================================

int64_t AbstractOrderbookDataSource::sumTotals() const
{
    return _sumTotals;
}

//==============================================================================

AbstractOrderbookDataSource::OrdersType AbstractOrderbookDataSource::type() const
{
    return _type;
}

//==============================================================================

bool AbstractOrderbookDataSource::isOrdersSet() const
{
    return _ordersSet;
}

//==============================================================================

void AbstractOrderbookDataSource::updateOrders(
    std::vector<OrderSummary> added, const std::vector<OrderSummary> removed)
{
    if (!_ordersSet) {
        return;
    }

    std::vector<UpdateEvent> changes;
    for (auto&& order : std::move(added)) {
        auto it = std::lower_bound(std::begin(_orders), std::end(_orders), order.price,
            [this](const auto& lhs, const auto& value) {
                return _type == OrdersType::Buy ? lhs.price > value : lhs.price < value;
            });

        adjustTotalOrderAdded(order);

        if (it != std::end(_orders) && it->price == order.price) {
            it->amount += order.amount;
            changes.emplace_back(OrderChangedEvent{ *it });
        } else {
            _orders.insert(it, order);
            changes.emplace_back(OrderAddedEvent{ order });
        }
    }

    for (auto&& order : std::move(removed)) {
        auto it = std::lower_bound(std::begin(_orders), std::end(_orders), order.price,
            [this](const auto& lhs, const auto& value) {
                return _type == OrdersType::Buy ? lhs.price > value : lhs.price < value;
            });

        if (it != std::end(_orders) && it->price == order.price) {
            adjustTotalOrderRemoved(order);
            it->amount -= order.amount;
            if (it->amount <= 0) {
                _orders.erase(it);
                changes.emplace_back(OrderRemovedEvent{ order });
            } else {
                changes.emplace_back(OrderChangedEvent{ *it });
            }
        }
    }

    updateEvent(changes);
}

//==============================================================================

void AbstractOrderbookDataSource::setOrders(AbstractOrderbookDataSource::Orders newOrders)
{
    _orders.swap(newOrders);
    _ordersSet = true;
    _sumAmounts = _sumTotals = 0;
    std::for_each(std::begin(_orders), std::end(_orders),
        std::bind(
            &AbstractOrderbookDataSource::adjustTotalOrderAdded, this, std::placeholders::_1));
    fetched();
}

//==============================================================================

void AbstractOrderbookDataSource::adjustTotalOrderAdded(const OrderSummary& order)
{
    _sumAmounts += order.amount;
    _sumTotals += static_cast<int64_t>(
        std::round(order.amount * (static_cast<double>(order.price) / COIN)));
}

//==============================================================================

void AbstractOrderbookDataSource::adjustTotalOrderRemoved(const OrderSummary& order)
{
    _sumAmounts -= order.amount;
    _sumTotals -= static_cast<int64_t>(
        std::round(order.amount * (static_cast<double>(order.price) / COIN)));
}

//==============================================================================
