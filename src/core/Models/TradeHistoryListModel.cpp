#include "TradeHistoryListModel.hpp"
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/TradingOrdersModel.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

static Trade Convert(io::stakenet::orderbook::protos::Trade trade)
{
    return Trade(QString::fromStdString(trade.id()), orderbook::ConvertBigInt(trade.size()),
        orderbook::ConvertBigInt(trade.price()), QDateTime::fromMSecsSinceEpoch(trade.executedon()),
        QString::fromStdString(trade.executingorderside()).toLower() != "buy");
}

//==============================================================================

TradeHistoryListModel::TradeHistoryListModel(
    orderbook::TradingOrdersModel* ordersModel, QString pairId, QObject* parent)
    : QAbstractListModel(parent)
    , _ordersModel(ordersModel)
    , _pairId(pairId.toStdString())
{
    init();
}

//==============================================================================

TradeHistoryListModel::~TradeHistoryListModel() {}

//==============================================================================

int TradeHistoryListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(_trades.size());
}

//==============================================================================

QVariant TradeHistoryListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }

    auto& trade = _trades.at(static_cast<size_t>(index.row()));

    switch (role) {
    case IdRole:
        return trade.id;
    case AmountRole:
        return static_cast<double>(trade.amount);
        //    case TimeRole: return trade.time.toString("hh:mm:ss");
    case TimeRole:
        return QVariant::fromValue(trade.time.toMSecsSinceEpoch());
    case PriceRole:
        return static_cast<double>(trade.price);
    case IsSoldRole:
        return trade.isSold;
    case ExistingOrderId:
        return QString();
    case ExecutingOrderId:
        return QString();
    case ExecutingOrderSide:
        return QString();
    default:
        break;
    }

    return QVariant();
}

//==============================================================================

QHash<int, QByteArray> TradeHistoryListModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[IdRole] = "id";
        roles[AmountRole] = "amount";
        roles[TimeRole] = "time";
        roles[PriceRole] = "price";
        roles[IsSoldRole] = "isSold";
        roles[ExistingOrderId] = "existingOrderId";
        roles[ExecutingOrderId] = "executingOrderId";
        roles[ExecutingOrderSide] = "executingOrderSide";
    }

    return roles;
}

//==============================================================================

void TradeHistoryListModel::onHistoricTradesFetched()
{
    QPointer<TradeHistoryListModel> self(this);

    auto lastTradeId = _lastTradeId;

    auto promise = Promise<orderbook::TradingOrdersModel::HistoricTrades>(
        [this, lastTradeId](const auto& resolve, const auto&) {
            QMetaObject::invokeMethod(
                _ordersModel, [=] { resolve(_ordersModel->historicTrades(_pairId, lastTradeId)); });
        });

    promise.then([self, lastTradeId](orderbook::TradingOrdersModel::HistoricTrades trades) {
        if (!self) {
            return;
        }

        if (trades.empty()) {
            self->updateIfCanFetchMore();
            return;
        }

        auto firstId = QString::fromStdString(trades.begin()->id());
        auto it = std::find_if(std::begin(self->_trades), std::end(self->_trades),
            [firstId](const auto& trade) { return trade.id == firstId; });

        if (it != std::end(self->_trades)) {
            return;
        }

        auto rows = self->rowCount();
        self->beginInsertRows(QModelIndex(), rows, rows + trades.size() - 1);
        self->_trades.resize(rows + trades.size());
        std::transform(
            std::begin(trades), std::end(trades), std::begin(self->_trades) + rows, Convert);
        self->_lastTradeId = trades.back().id();
        self->endInsertRows();
    });
}

//==============================================================================

void TradeHistoryListModel::onHistoricTradeAdded(Trade trade)
{
    beginInsertRows(QModelIndex(), 0, 0);
    _trades.insert(std::begin(_trades), trade);
    endInsertRows();
}

//==============================================================================

void TradeHistoryListModel::init()
{
    connect(_ordersModel, &orderbook::TradingOrdersModel::historicTradeAdded, this,
        [this](auto pairId, auto trade) {
            if (_pairId == pairId) {
                this->onHistoricTradeAdded(Convert(trade));
            }
        });
}

//==============================================================================

void TradeHistoryListModel::updateIfCanFetchMore()
{

    QPointer<TradeHistoryListModel> self(this);

    auto promise = Promise<bool>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(
            _ordersModel, [=] { resolve(_ordersModel->canFetchMore(_pairId)); });
    });

    promise.then([self](bool canFetch) {
        if (!self) {
            return;
        }
        self->_canFetchMore = canFetch;
    });
}

//==============================================================================

void TradeHistoryListModel::fetchMore(const QModelIndex&)
{
    QPointer<TradeHistoryListModel> self(this);

    auto promise = Promise<void>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_ordersModel,
            [=] { _ordersModel->fetchMoreHistoricTrades(_pairId).then([resolve] { resolve(); }); });
    })
                       .then([self] {
                           if (!self) {
                               return;
                           }

                           self->onHistoricTradesFetched();
                       });
}

//==============================================================================

bool TradeHistoryListModel::canFetchMore(const QModelIndex&) const
{
    return _canFetchMore;
}

//==============================================================================

Trade::Trade(QString id, Balance amount, Balance price, QDateTime time, bool isSold)
    : id(id)
    , amount(amount)
    , price(price)
    , time(time)
    , isSold(isSold)
{
}

//==============================================================================
