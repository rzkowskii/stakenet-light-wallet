#ifndef OrderBookModel_HPP
#define OrderBookModel_HPP

#include <Data/AbstractOrderbookDataSource.hpp>
#include <QAbstractListModel>
#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>

class OrderBookListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(double bestPrice READ bestPrice NOTIFY bestPriceChanged)
    Q_PROPERTY(unsigned count READ count NOTIFY countChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QVariantMap totals READ totals NOTIFY totalOrderBookAmountChanged)

public:
    enum Roles { SumRole, AmountRole, PriceRole, IsOwnOrderRole };
    Q_ENUMS(Roles)

    struct OrderListModelEntry : OrderSummary {
        using OrderSummary::OrderSummary;
        OrderListModelEntry(const OrderSummary& summary, bool hasOwnOrder)
            : OrderSummary(summary)
            , hasOwnOrder(hasOwnOrder)
        {
        }

        Balance sum{ 0 };
        bool hasOwnOrder{ false };
    };

    explicit OrderBookListModel(QPointer<AbstractOrderbookDataSource> orderBookData, QString pairId,
        QObject* parent = nullptr);
    virtual ~OrderBookListModel() override;

    double bestPrice() const;
    unsigned count() const;
    bool loading() const;
    QVariantMap totals() const;

    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const override final;
    virtual QVariant data(const QModelIndex& index, int role) const override final;
    virtual QHash<int, QByteArray> roleNames() const override final;

signals:
    void totalOrderBookAmountChanged();
    void bestPriceChanged();
    void countChanged();
    void loadingChanged();

protected slots:
    virtual QVariantMap get(int row);
    virtual void onOrdersChanged() = 0;
    virtual void onOrdersUpdated(
        const std::vector<AbstractOrderbookDataSource::UpdateEvent>& updates)
        = 0;

protected:
    void setBestPrice(Balance price);
    void setLoading(bool value);

private:
    void init();

protected:
    QPointer<AbstractOrderbookDataSource> _orderBookData;
    std::vector<OrderListModelEntry> _orders;
    QString _pairId;
    Balance _bestPrice{ 0 };
    bool _loading{ true };
};

#endif // OrderBookModel_HPP
