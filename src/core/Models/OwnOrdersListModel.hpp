#ifndef OWNORDERSLISTMODEL_HPP
#define OWNORDERSLISTMODEL_HPP

#include <Data/OwnOrdersDataSource.hpp>

#include <QAbstractListModel>
#include <QObject>

class OwnOrdersListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(double buyTotalAmount READ buyTotalAmount NOTIFY totalAmountsChanged)
    Q_PROPERTY(double sellTotalAmount READ sellTotalAmount NOTIFY totalAmountsChanged)
    Q_PROPERTY(double totalSell READ totalSell NOTIFY totalAmountsChanged)
    Q_PROPERTY(double totalBuy READ totalBuy NOTIFY totalAmountsChanged)
public:
    enum Roles {
        IdRole,
        SumRole,
        AmountRole,
        FilledPercentageRole,
        OpenAmountRole,
        CompletedAmontRole,
        TypeRole,
        SideRole,
        PairIdRole,
        PriceRole
    };

    explicit OwnOrdersListModel(
        QPointer<OwnOrdersDataSource> orderBookData, QString pairId, QObject* parent = nullptr);

    double buyTotalAmount() const;
    double sellTotalAmount() const;
    double totalSell() const;
    double totalBuy() const;

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

signals:
    void totalAmountsChanged();
    void ownOrderAdded();

    // OrderBookListModel interface

private slots:
    void onOrdersChanged();
    void onOrderAdded(const OwnOrderEntry& order);
    void onOrderChanged(const OwnOrderEntry& order);
    void onOrderRemoved(const OwnOrderEntry& order);

private:
    void updateTotalAmounts();

private:
    QPointer<OwnOrdersDataSource> _orderBookData;
    std::vector<OwnOrderEntry> _orders;
    Balance _buyTotalAmount{ 0 };
    Balance _sellTotalAmount{ 0 };
    Balance _totalSell{ 0 };
    Balance _totalBuy{ 0 };
};

#endif // OWNORDERSLISTMODEL_HPP
