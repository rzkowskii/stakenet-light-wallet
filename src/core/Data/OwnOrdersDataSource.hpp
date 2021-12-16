#ifndef OWNORDERSDATASOURCE_HPP
#define OWNORDERSDATASOURCE_HPP

#include <Data/AbstractOrderbookDataSource.hpp>
#include <QObject>

//==============================================================================

namespace orderbook {
class OrderbookClient;
}

//==============================================================================

struct OwnOrderEntry : OrderSummary {
    OwnOrderEntry() = default;
    OwnOrderEntry(OrderSummary summary)
        : OrderSummary(summary)
    {
    }

    using OrderPortion = std::tuple<QString, Balance>;
    QString id;
    QString pairId;
    Enums::OrderSide side;
    std::vector<OrderPortion> openPortions;
    std::vector<OrderPortion> closedPortions;
    Balance openAmount{ 0 };
    Balance completedAmount{ 0 };
};

//==============================================================================

class OwnOrdersDataSource : public QObject {
    Q_OBJECT
public:
    using Orders = std::vector<OwnOrderEntry>;
    explicit OwnOrdersDataSource(
        QPointer<orderbook::OrderbookClient> orderbook, QString pairId, QObject* parent = nullptr);

    void fetch();
    const Orders& orders() const;

signals:
    void fetched();
    void orderAdded(const OwnOrderEntry& order);
    void orderRemoved(const OwnOrderEntry& order);
    void orderChanged(const OwnOrderEntry& order);

private slots:
    void onOwnOrderPlaced(OwnOrderEntry entry);
    void onOwnOrderChanged(OwnOrderEntry entry);
    void onOwnOrderMatched(std::string localId);
    void onOwnOrderCanceled(std::string orderId);
    void onOwnOrdersChanged(std::string pairId);

private:
    void setOrders(Orders newOrders);

private:
    Orders _orders;
    QPointer<orderbook::OrderbookClient> _orderbook;
    std::string _pairId;
};

//==============================================================================

#endif // OWNORDERSDATASOURCE_HPP
