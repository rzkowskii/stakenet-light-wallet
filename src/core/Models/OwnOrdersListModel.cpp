#include "OwnOrdersListModel.hpp"
#include <Data/OwnOrdersDataSource.hpp>

static double CalculateOrderFilledAmountPercentage(Balance completed, Balance total)
{
    return completed / static_cast<double>(total) * 100.0;
}

//==============================================================================

OwnOrdersListModel::OwnOrdersListModel(
    QPointer<OwnOrdersDataSource> orderBookData, QString pairId, QObject* parent)
    : QAbstractListModel(parent)
    , _orderBookData(orderBookData)
{
    connect(
        orderBookData, &OwnOrdersDataSource::fetched, this, &OwnOrdersListModel::onOrdersChanged);
    connect(
        orderBookData, &OwnOrdersDataSource::orderAdded, this, &OwnOrdersListModel::onOrderAdded);
    connect(orderBookData, &OwnOrdersDataSource::orderChanged, this,
        &OwnOrdersListModel::onOrderChanged);
    connect(orderBookData, &OwnOrdersDataSource::orderRemoved, this,
        &OwnOrdersListModel::onOrderRemoved);
}

//==============================================================================

double OwnOrdersListModel::buyTotalAmount() const
{
    return static_cast<double>(_buyTotalAmount);
}

//==============================================================================

double OwnOrdersListModel::sellTotalAmount() const
{
    return static_cast<double>(_sellTotalAmount);
}

//==============================================================================

double OwnOrdersListModel::totalSell() const
{
    return static_cast<double>(_totalSell);
}

//==============================================================================

double OwnOrdersListModel::totalBuy() const
{
    return static_cast<double>(_totalBuy);
}

//==============================================================================

int OwnOrdersListModel::rowCount(const QModelIndex& /*parent*/) const
{
    return _orders.size();
}

//==============================================================================

QVariant OwnOrdersListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }

    const size_t row = static_cast<size_t>(index.row());
    const auto& order = _orders.at(row);

    switch (role) {
    case IdRole:
        return order.id;
    case AmountRole:
        return static_cast<double>(order.amount);
    case FilledPercentageRole:
        return CalculateOrderFilledAmountPercentage(
            order.completedAmount, order.completedAmount + order.openAmount);
    case OpenAmountRole:
        return static_cast<double>(order.openAmount);
    case CompletedAmontRole:
        return static_cast<double>(order.completedAmount);
    case PairIdRole:
        return order.pairId;
    case PriceRole:
        return static_cast<double>(order.price);
    case SideRole:
        return static_cast<int>(order.side);
    default:
        break;
    }
    return QVariant();
}

//==============================================================================

QHash<int, QByteArray> OwnOrdersListModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[IdRole] = "id";
        roles[SumRole] = "sum";
        roles[AmountRole] = "amount";
        roles[TypeRole] = "type";
        roles[FilledPercentageRole] = "filledPercentage";
        roles[OpenAmountRole] = "open";
        roles[CompletedAmontRole] = "completed";
        roles[PairIdRole] = "pairId";
        roles[PriceRole] = "price";
        roles[SideRole] = "side";
    }

    return roles;
}

//==============================================================================

void OwnOrdersListModel::onOrdersChanged()
{
    beginResetModel();
    _orders = _orderBookData->orders();
    endResetModel();
    updateTotalAmounts();
}

//==============================================================================

void OwnOrdersListModel::onOrderAdded(const OwnOrderEntry& order)
{
    beginInsertRows(QModelIndex(), rowCount(), rowCount());
    _orders.emplace_back(order);
    endInsertRows();
    updateTotalAmounts();
    ownOrderAdded();
}

//==============================================================================

void OwnOrdersListModel::onOrderChanged(const OwnOrderEntry& order)
{
    auto it = std::find_if(_orders.begin(), _orders.end(),
        [orderId = order.id](const auto& order) { return order.id == orderId; });

    if (it != std::end(_orders)) {
        *it = order;
        auto modelIndex = index(std::distance(std::begin(_orders), it));
        dataChanged(modelIndex, modelIndex);
        updateTotalAmounts();
    }
}

//==============================================================================

void OwnOrdersListModel::onOrderRemoved(const OwnOrderEntry& order)
{
    auto it = std::find_if(_orders.begin(), _orders.end(),
        [orderId = order.id](const auto& order) { return order.id == orderId; });

    if (it != std::end(_orders)) {
        auto index = static_cast<int>(std::distance(_orders.begin(), it));
        beginRemoveRows(QModelIndex(), index, index);
        _orders.erase(it);
        endRemoveRows();
        updateTotalAmounts();
    }
}

//==============================================================================

void OwnOrdersListModel::updateTotalAmounts()
{
    _sellTotalAmount = _buyTotalAmount = _totalSell = _totalBuy = 0;
    for (auto&& order : _orders) {
        if (order.side == Enums::OrderSide::Buy) {
            _buyTotalAmount
                += static_cast<Balance>(order.amount * (static_cast<double>(order.price) / COIN));
            _totalBuy += order.amount;
        } else {
            _sellTotalAmount += order.amount;
            _totalSell
                += static_cast<Balance>(order.amount * (static_cast<double>(order.price) / COIN));
        }
    }

    totalAmountsChanged();
}

//==============================================================================
