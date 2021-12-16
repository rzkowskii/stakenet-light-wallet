#include "CurrencyModel.hpp"
#include <Data/LocalCurrency.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

CurrencyModel::CurrencyModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

//==============================================================================

CurrencyModel::~CurrencyModel() {}

//==============================================================================

int CurrencyModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(_currencies.size());
}

//==============================================================================

QVariant CurrencyModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }

    const auto currency = _currencies.at(static_cast<size_t>(index.row()));

    switch (role) {
    case CodeRole:
        return currency.code;
    case NameRole:
        return currency.name;
    case SymbolRole:
        return currency.symbol;
    default:
        break;
    }

    return QVariant();
}

//==============================================================================

QHash<int, QByteArray> CurrencyModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[CodeRole] = "code";
        roles[NameRole] = "name";
        roles[SymbolRole] = "symbol";
    }

    return roles;
}

//==============================================================================

void CurrencyModel::initialize(QObject* appViewModel)
{
    if (auto viewModel = qobject_cast<ApplicationViewModel*>(appViewModel)) {
        initCurrency(viewModel->localCurrency());
    } else {
        Q_ASSERT_X(
            false, "CurrencyModel::initialize", "No ApplicationViewModel, something is wrong");
    }
}

//==============================================================================

QString CurrencyModel::getCode(int index)
{
    if (_currencies.size() > 0)
        return _currencies.at(static_cast<size_t>(index)).code;
    else
        return QString();
}

//==============================================================================

int CurrencyModel::getInitial(QString code)
{
    auto it = std::find_if(std::begin(_currencies), std::end(_currencies),
        [code](const CurrencyEntry& currency) { return currency.code == code; });
    return std::distance(std::begin(_currencies), it);
}

//==============================================================================

void CurrencyModel::initCurrency(LocalCurrency* localCurrency)
{
    beginResetModel();

    _localCurrency = localCurrency;
    _currencies = localCurrency->currencies();

    endResetModel();
}

//==============================================================================
