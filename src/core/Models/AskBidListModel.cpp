#include "AskBidListModel.hpp"
#include <Data/OwnOrdersDataSource.hpp>
#include <Utils/Logging.hpp>

static const size_t MAX_FETCH_BATCH = 20;

//==============================================================================

AskBidListModel::AskBidListModel(QPointer<AbstractOrderbookDataSource> orderBookData,
    QPointer<OwnOrdersDataSource> ownOrderBookData, QString pairId, QObject* parent)
    : OrderBookListModel(orderBookData, pairId, parent)
{
    connect(ownOrderBookData, &OwnOrdersDataSource::orderAdded, this, [this](const auto& order) {
        if ((order.side == Enums::OrderSide::Buy && _orderBookData->type() == AbstractOrderbookDataSource::OrdersType::Buy)
                || (order.side == Enums::OrderSide::Sell && _orderBookData->type() == AbstractOrderbookDataSource::OrdersType::Sell)) {
            this->onOwnOrderAdded(order);
        }
    });

    connect(ownOrderBookData, &OwnOrdersDataSource::orderRemoved, this, [this](const auto& order) {
        if ((order.side == Enums::OrderSide::Buy && _orderBookData->type() == AbstractOrderbookDataSource::OrdersType::Buy)
             || (order.side == Enums::OrderSide::Sell && _orderBookData->type() == AbstractOrderbookDataSource::OrdersType::Sell)) {
            auto it = _ownOrders.find(order.price);
            if (it != std::end(_ownOrders)) {
                this->onOwnOrderRemoved(order);
            }
        }
    });
}

//==============================================================================

void AskBidListModel::fetchMore(const QModelIndex& parent)
{
    Q_UNUSED(parent)
    auto toFetch
        = std::min<size_t>(_orderBookData->orders().size() - _ordersCount, MAX_FETCH_BATCH);
    fetchOrders(_ordersCount, toFetch);
}

//==============================================================================

bool AskBidListModel::canFetchMore(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return _ordersCount < _orderBookData->orders().size();
}

//==============================================================================

void AskBidListModel::onOrdersChanged()
{
    beginResetModel();
    _orders.clear();
    _ordersCount = 0;
    endResetModel();
}

//==============================================================================

void AskBidListModel::onOrdersUpdated(
    const std::vector<AbstractOrderbookDataSource::UpdateEvent>& updates)
{
    size_t ordersAdded = 0;
    size_t ordersRemoved = 0;
    size_t minUpdatedRow = std::numeric_limits<size_t>::max();
    auto oldSize = _orders.size();

    auto comparator = [this](const auto& lhs, const auto& value) {
        return _orderBookData->type() == AbstractOrderbookDataSource::OrdersType::Buy
            ? lhs.price > value
            : lhs.price < value;
    };

    for (auto&& orderEvent : updates) {
        if (auto event = boost::get<AbstractOrderbookDataSource::OrderRemovedEvent>(&orderEvent)) {
            auto price = event->order.price;

            auto it = std::lower_bound(std::begin(_orders), std::end(_orders), price, comparator);

            if (it != std::end(_orders) && it->price == price) {
                auto erasedOrderIndex = std::distance(std::begin(_orders), it);
                minUpdatedRow = std::min<size_t>(minUpdatedRow, erasedOrderIndex);
                _orders.erase(it);
                ++ordersRemoved;
                --_ordersCount;
            }

        } else if (auto event
            = boost::get<AbstractOrderbookDataSource::OrderAddedEvent>(&orderEvent)) {
            const auto& order = event->order;
            auto it
                = std::lower_bound(std::begin(_orders), std::end(_orders), order.price, comparator);

            // means that order was not fetched, just skip
            if (it == std::end(_orders)) {
                if (_orderBookData->orders().size() - _ordersCount > MAX_FETCH_BATCH) {
                    continue;
                }
            }

            bool isOwnOrder = _ownOrders.count(order.price);

            auto rows = std::distance(std::begin(_orders), it);
            minUpdatedRow = std::min<size_t>(minUpdatedRow, rows);
            ++_ordersCount;

            OrderListModelEntry entry{ order, isOwnOrder };
            _orders.insert(it, entry);
            ++ordersAdded;
        } else if (auto event
            = boost::get<AbstractOrderbookDataSource::OrderChangedEvent>(&orderEvent)) {
            const auto& order = event->order;
            auto it
                = std::lower_bound(std::begin(_orders), std::end(_orders), order.price, comparator);
            if (it != std::end(_orders) && it->price == order.price) {
                auto changedOrderIndex = std::distance(std::begin(_orders), it);
                minUpdatedRow = std::min<size_t>(minUpdatedRow, changedOrderIndex);
                it->amount = order.amount;
            }
        }
    }

    if (ordersRemoved > ordersAdded) {
        beginRemoveRows(QModelIndex(), rowCount(), oldSize - 1);
        endRemoveRows();
    } else if (ordersRemoved < ordersAdded) {
        beginInsertRows(QModelIndex(), oldSize, rowCount() - 1);
        endInsertRows();
    }

    updateBestPrice();
    calculateOrdersTotals(minUpdatedRow, true);
    totalOrderBookAmountChanged();
}

//==============================================================================

void AskBidListModel::onOwnOrderAdded(const OrderSummary& added)
{
    auto comparator = [this](const auto& lhs, const auto& value) {
        return _orderBookData->type() == AbstractOrderbookDataSource::OrdersType::Buy
            ? lhs.price > value
            : lhs.price < value;
    };

    auto it = std::lower_bound(std::begin(_orders), std::end(_orders), added.price, comparator);

    _ownOrders[added.price] += added.amount;

    if (it != std::end(_orders) && it->price == added.price && !it->hasOwnOrder) {
        it->hasOwnOrder = true;
        auto modelIndex = index(std::distance(std::begin(_orders), it));
        dataChanged(modelIndex, modelIndex, { IsOwnOrderRole });
    }
}

//==============================================================================

void AskBidListModel::onOwnOrderRemoved(const OrderSummary& removed)
{
    bool hasOwnOrder = false;

    {
        auto it = _ownOrders.find(removed.price);
        if (it != std::end(_ownOrders)) {
            it->second -= removed.amount;
            if (it->second <= 0) {
                _ownOrders.erase(it);
            } else {
                hasOwnOrder = true;
            }
        }
    }

    if (!hasOwnOrder) {
        auto comparator = [this](const auto& lhs, const auto& value) {
            return _orderBookData->type() == AbstractOrderbookDataSource::OrdersType::Buy
                ? lhs.price > value
                : lhs.price < value;
        };

        auto it
            = std::lower_bound(std::begin(_orders), std::end(_orders), removed.price, comparator);

        if (it != std::end(_orders) && it->price == removed.price) {
            it->hasOwnOrder = false;
            auto modelIndex = index(std::distance(std::begin(_orders), it));
            dataChanged(modelIndex, modelIndex, { IsOwnOrderRole });
        }
    }
}

//==============================================================================

void AskBidListModel::calculateOrdersTotals(size_t from, bool notify)
{
    for (size_t i = from; i < _orders.size(); ++i) {
        _orders[i].sum = _orders[i].amount * _orders[i].price;
        if (i > 0) {
            _orders[i].sum += _orders[i - 1].sum;
        }
    }

    auto count = rowCount();

    if (notify && count > 0 && from < count) {
        auto fromIndex = index(static_cast<int>(from));
        dataChanged(fromIndex, index(count - 1));
    }
}

//==============================================================================

void AskBidListModel::updateBestPrice()
{
    if (_orders.empty()) {
        setBestPrice(0);
        return;
    }

    setBestPrice(_orders.front().price);
}

//==============================================================================

void AskBidListModel::fetchOrders(size_t from, size_t count)
{
    const auto& orders = _orderBookData->orders();
    const auto to = std::min(from + std::min(count, MAX_FETCH_BATCH), orders.size());
    auto oldSize = _orders.size();

    Q_ASSERT(from >= _ordersCount);

    for (size_t i = from; i < to; ++i) {
        const auto& order = orders.at(i);
        bool isOwnOrder = _ownOrders.count(order.price);

        _orders.emplace_back(order, isOwnOrder);
    }

    _ordersCount += (to - from);

    beginInsertRows(QModelIndex(), oldSize, _orders.size() - 1);
    updateBestPrice();
    calculateOrdersTotals(oldSize);
    totalOrderBookAmountChanged();
    endInsertRows();
}

//==============================================================================
