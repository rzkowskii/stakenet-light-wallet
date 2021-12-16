#ifndef ABSTRACTORDERBOOKDATASOURCE_HPP
#define ABSTRACTORDERBOOKDATASOURCE_HPP

#include <QObject>
#include <boost/variant.hpp>
#include <unordered_map>

#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

//==============================================================================

struct OrderSummary {
    OrderSummary() = default;
    OrderSummary(Balance price, Balance amount)
        : price(price)
        , amount(amount)
    {
    }

    Balance price{ 0 };
    Balance amount{ 0 };
};

//==============================================================================

class AbstractOrderbookDataSource : public QObject {
    Q_OBJECT
public:
    using Orders = std::vector<OrderSummary>;
    enum class OrdersType { Buy, Sell };

    struct OrderAddedEvent {
        OrderSummary order;
    };

    struct OrderRemovedEvent {
        OrderSummary order;
    };

    struct OrderChangedEvent {
        OrderSummary order;
    };

    using UpdateEvent = boost::variant<OrderAddedEvent, OrderRemovedEvent, OrderChangedEvent>;
    using UpdatesBatch = std::vector<UpdateEvent>;

    explicit AbstractOrderbookDataSource(
        OrdersType type, QString pairId, QObject* parent = nullptr);

    virtual void fetch() = 0;

    QString pairId() const;
    const Orders& orders() const;
    bool isOrdersSet() const;
    int64_t sumAmounts() const;
    int64_t sumTotals() const;
    OrdersType type() const;

signals:
    void fetched();
    void updateEvent(const std::vector<UpdateEvent>& changes);

public slots:

protected:
    void updateOrders(std::vector<OrderSummary> added, const std::vector<OrderSummary> removed);
    void setOrders(Orders newOrders);

protected:
    std::string _pairId;

private:
    void adjustTotalOrderAdded(const OrderSummary& order);
    void adjustTotalOrderRemoved(const OrderSummary& order);

private:
    Orders _orders;
    OrdersType _type;
    int64_t _sumAmounts{ 0 };
    int64_t _sumTotals{ 0 };
    bool _ordersSet{ false };
};

//==============================================================================

#endif // ABSTRACTORDERBOOKDATASOURCE_HPP
