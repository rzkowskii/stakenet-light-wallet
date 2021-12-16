#include "AssetsListProxyModel.hpp"

//==============================================================================

QMLSortFilterListProxyModel::QMLSortFilterListProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent)
{
    connect(this, &QSortFilterProxyModel::rowsInserted, this,
        &QMLSortFilterListProxyModel::countChanged);
    connect(this, &QSortFilterProxyModel::rowsRemoved, this,
        &QMLSortFilterListProxyModel::countChanged);
}

//==============================================================================

QObject* QMLSortFilterListProxyModel::source() const
{
    return sourceModel();
}

//==============================================================================

void QMLSortFilterListProxyModel::setSource(QObject* source)
{
    setSourceModel(qobject_cast<QAbstractItemModel*>(source));
}

//==============================================================================

QByteArray QMLSortFilterListProxyModel::filterRole() const
{
    return _filterRole;
}

//==============================================================================

void QMLSortFilterListProxyModel::setFilterRole(const QByteArray& role)
{
    if (_filterRole != role) {
        _filterRole = role;
        QSortFilterProxyModel::setFilterRole(roleKey(role));
    }
}

//==============================================================================

QString QMLSortFilterListProxyModel::filterString() const
{
    return filterRegExp().pattern();
}

//==============================================================================

void QMLSortFilterListProxyModel::setFilterString(const QString& filter)
{
    setFilterRegExp(QRegExp(filter, filterCaseSensitivity(),
        static_cast<QRegExp::PatternSyntax>(filterRegExp().patternSyntax())));
}

//==============================================================================

QByteArray QMLSortFilterListProxyModel::sortRole() const
{
    return _sortRole;
}

//==============================================================================

void QMLSortFilterListProxyModel::setSortRole(const QByteArray& role)
{
    if (_sortRole != role) {
        _sortRole = role;
        QSortFilterProxyModel::setSortRole(roleKey(role));
        sort(0, sortOrder());
    }
}

//==============================================================================

bool QMLSortFilterListProxyModel::sortAsc() const
{
    return sortOrder() == Qt::SortOrder::AscendingOrder;
}

//==============================================================================

void QMLSortFilterListProxyModel::setSortAsc(bool value)
{
    sort(0, value ? Qt::SortOrder::AscendingOrder : Qt::SortOrder::DescendingOrder);
}

//==============================================================================

int QMLSortFilterListProxyModel::count() const
{
    return rowCount();
}

//==============================================================================

QVariantMap QMLSortFilterListProxyModel::get(int row)
{
    QHash<int, QByteArray> names = roleNames();
    QHashIterator<int, QByteArray> it(names);
    QVariantMap result;
    while (it.hasNext()) {
        it.next();
        QModelIndex idx = mapToSource(index(row, 0));
        QVariant data = idx.data(it.key());
        result[it.value()] = data;
    }
    return result;
}

//==============================================================================

int QMLSortFilterListProxyModel::roleKey(const QByteArray& role) const
{
    QHash<int, QByteArray> roles = roleNames();
    QHashIterator<int, QByteArray> it(roles);
    while (it.hasNext()) {
        it.next();
        if (it.value() == role)
            return it.key();
    }
    return -1;
}

//==============================================================================

bool QMLSortFilterListProxyModel::filterAcceptsRow(
    int sourceRow, const QModelIndex& sourceParent) const
{
    QRegExp rx = filterRegExp();
    if (rx.isEmpty())
        return true;
    QAbstractItemModel* model = sourceModel();
    QModelIndex sourceIndex = model->index(sourceRow, 0, sourceParent);
    if (!filterRole().isEmpty()) {
        QString key = model->data(sourceIndex, roleKey(filterRole())).toString();
        if (key.contains(rx)) {
            return true;
        }
    } else {
        QHash<int, QByteArray> roles = roleNames();
        QHashIterator<int, QByteArray> it(roles);
        while (it.hasNext()) {
            it.next();
            QString key = model->data(sourceIndex, it.key()).toString();
            if (key.contains(rx)) {
                return true;
            }
        }
    }
    return false;
}

//==============================================================================

bool QMLSortFilterListProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    return left.data(roleKey(_sortRole)) < right.data(roleKey(_sortRole));
}

//==============================================================================
