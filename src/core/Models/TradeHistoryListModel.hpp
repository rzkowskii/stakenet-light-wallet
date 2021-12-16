#ifndef TRADEHISTORYLISTMODEL_HPP
#define TRADEHISTORYLISTMODEL_HPP

#include <QAbstractListModel>
#include <QDateTime>
#include <QPointer>
#include <Tools/Common.hpp>

namespace orderbook {
class TradingOrdersModel;
}

struct Trade {
    Trade() = default;
    Trade(QString id, Balance amount, Balance price, QDateTime time, bool isSold);

    QString id;
    Balance amount;
    Balance price;
    QDateTime time;
    bool isSold = false;
};

class TradeHistoryListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        IdRole,
        AmountRole,
        TimeRole,
        PriceRole,
        IsSoldRole,
        ExistingOrderId,
        ExecutingOrderId,
        ExecutingOrderSide
    };
    Q_ENUMS(Roles)

    explicit TradeHistoryListModel(
        orderbook::TradingOrdersModel* ordersModel, QString pairId, QObject* parent = nullptr);
    ~TradeHistoryListModel() override;

    void fetchMore(const QModelIndex& parent) override;
    bool canFetchMore(const QModelIndex& parent) const override;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override final;
    virtual QVariant data(const QModelIndex& index, int role) const override final;
    virtual QHash<int, QByteArray> roleNames() const override final;

public slots:

private slots:
    void onHistoricTradesFetched();
    void onHistoricTradeAdded(Trade trade);

private:
    void init();
    void updateIfCanFetchMore();

private:
    QPointer<orderbook::TradingOrdersModel> _ordersModel;
    std::vector<Trade> _trades;
    std::string _pairId;
    std::string _lastTradeId;
    bool _canFetchMore{ true };
};

#endif // TRADEHISTORYLISTMODEL_HPP
