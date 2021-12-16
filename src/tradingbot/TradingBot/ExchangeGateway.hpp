#ifndef EXCHANGEGATEWAY_HPP
#define EXCHANGEGATEWAY_HPP

#include <QObject>
#include <QString>
#include <variant>

#include <Orderbook/Types.hpp>
#include <Utils/Utils.hpp>

namespace tradingbot {
namespace gateway {
    struct ExecutedOwnOrder {
        QString orderId;
        QString localId;
        QString pairId;
        int64_t price;
        int64_t quantity;
    };

    using PlaceOrderOutcome = std::variant<ExecutedOwnOrder, orderbook::OwnOrder>;
    class ExchangeGateway : public QObject {
        Q_OBJECT
    public:
        ExchangeGateway(QObject* parent = nullptr);
        virtual ~ExchangeGateway();

        virtual double getLastPrice(QString pairId) const = 0;
        virtual Promise<PlaceOrderOutcome> placeOrder(orderbook::LimitOrder order) = 0;
        virtual Promise<void> cancelOrder(std::string pairId, std::string localId) const = 0;
        virtual boost::optional<orderbook::OwnOrder> tryGetOwnOrder(
            std::string pairId, std::string orderId) const = 0;

    signals:
        void orderPlaced(orderbook::OwnOrder order);
        void orderChanged(orderbook::OwnOrder order);
        void orderCompleted(std::string orderId);
    };
}
}

#endif // EXCHANGEGATEWAY_HPP
