#ifndef ASKBIDLISTMODEL_HPP
#define ASKBIDLISTMODEL_HPP

#include <Models/OrderBookListModel.hpp>
#include <QObject>
#include <unordered_set>

class OwnOrdersDataSource;

class AskBidListModel : public OrderBookListModel {
    Q_OBJECT
public:
    explicit AskBidListModel(QPointer<AbstractOrderbookDataSource> orderBookData,
        QPointer<OwnOrdersDataSource> ownOrderBookData, QString pairId, QObject* parent = nullptr);

    // QAbstractItemModel interface
public:
    void fetchMore(const QModelIndex& parent) override;
    bool canFetchMore(const QModelIndex& parent) const override;
signals:

public slots:

    // OrderBookListModel interface
protected slots:
    void onOrdersChanged() override;
    void onOrdersUpdated(
        const std::vector<AbstractOrderbookDataSource::UpdateEvent>& updates) override;

private slots:
    void onOwnOrderAdded(const OrderSummary& added);
    void onOwnOrderRemoved(const OrderSummary& removed);

private:
    void calculateOrdersTotals(size_t from = 0, bool notify = false);
    void updateBestPrice();
    void fetchOrders(size_t from, size_t count);

private:
    std::unordered_map<int64_t, int64_t> _ownOrders;
    size_t _ordersCount{ 0 };
};

#endif // ASKBIDLISTMODEL_HPP
