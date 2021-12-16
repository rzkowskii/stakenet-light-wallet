#include "OrderBookListModel.hpp"
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>
#include <numeric>

//==============================================================================

OrderBookListModel::OrderBookListModel(
    QPointer<AbstractOrderbookDataSource> orderBookData, QString pairId, QObject* parent)
    : QAbstractListModel(parent)
    , _orderBookData(orderBookData)
    , _pairId(pairId)
{
    init();
}

//==============================================================================

OrderBookListModel::~OrderBookListModel() {}

//==============================================================================

double OrderBookListModel::bestPrice() const
{
    return static_cast<double>(_bestPrice);
}

//==============================================================================

unsigned OrderBookListModel::count() const
{
    return static_cast<unsigned>(_orders.size());
}

//==============================================================================

bool OrderBookListModel::loading() const
{
    return _loading;
}

//==============================================================================

QVariantMap OrderBookListModel::totals() const
{
    QVariantMap totals;
    totals["sumAmountsStr"] = FormatAmount(_orderBookData->sumAmounts());
    totals["sumTotalStr"] = FormatAmount(_orderBookData->sumTotals());
    return totals;
}

//==============================================================================

int OrderBookListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(_orders.size());
}

//==============================================================================

QVariant OrderBookListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }

    const size_t row = static_cast<size_t>(index.row());
    const auto& order = _orders.at(row);

    switch (role) {
    case SumRole:
        return FormatAmount(order.sum);
    case AmountRole:
        return static_cast<double>(order.amount);
    case PriceRole:
        return static_cast<double>(order.price);
    case IsOwnOrderRole:
        return order.hasOwnOrder;
    default:
        break;
    }
    return QVariant();
}

//==============================================================================

QHash<int, QByteArray> OrderBookListModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[SumRole] = "sum";
        roles[AmountRole] = "amount";
        roles[PriceRole] = "price";
        roles[IsOwnOrderRole] = "isOwn";
    }

    return roles;
}

//==============================================================================

QVariantMap OrderBookListModel::get(int row)
{
    QHash<int, QByteArray> names = roleNames();
    QHashIterator<int, QByteArray> it(names);
    QVariantMap result;
    while (it.hasNext()) {
        it.next();
        QModelIndex idx = index(row);
        QVariant data = idx.data(it.key());
        result[it.value()] = data;
    }
    return result;
}

//==============================================================================

void OrderBookListModel::setBestPrice(Balance price)
{
    if (_bestPrice != price) {
        _bestPrice = price;
        bestPriceChanged();
    }
}

//==============================================================================

void OrderBookListModel::setLoading(bool value)
{
    if (_loading != value) {
        _loading = value;
        loadingChanged();
    }
}

//==============================================================================

void OrderBookListModel::init()
{
    connect(_orderBookData, &AbstractOrderbookDataSource::fetched, this, [this]() {
        setLoading(!_orderBookData->isOrdersSet());
        onOrdersChanged();
    });
    connect(_orderBookData, &AbstractOrderbookDataSource::updateEvent, this,
        &OrderBookListModel::onOrdersUpdated);
    setLoading(!_orderBookData->isOrdersSet());
}

//==============================================================================
