#ifndef MOVINGGRIDSTRATEGY_HPP
#define MOVINGGRIDSTRATEGY_HPP

#include <QObject>
#include <unordered_map>

#include <TradingBot/ExchangeGateway.hpp>

namespace tradingbot {

/*!
 * \brief The MovingGridStrategy class is a class that implements
 * logic for moving grid strategy. To initialize it user needs to pass:
 * `pairId` - pair that will be used for current strategy
 * `levels` - number of orders that will be posted for EACH bid/ask. Passing 5 means there will be
 * total 10 orders, 5 ASKs and 5 BIDs.
 */
class MovingGridStrategy : public QObject {
    Q_OBJECT
public:
    struct Config {
        QString pairId;
        uint32_t levels;
        double interval;
        uint32_t risk;
        double orderFee;
        uint32_t baseQuantity; // bot capacity for base currency
        uint32_t quoteQuantity; // bot capacity for quote currency
    };

    explicit MovingGridStrategy(
        Config cfg, std::unique_ptr<gateway::ExchangeGateway> gateway, QObject* parent = nullptr);

    Promise<void> start();
    Promise<void> stop();
    double gridPrice(double basePrice, bool isBuy, uint32_t level) const;
    uint32_t calculateQuantity(bool isBuy) const;

signals:

private slots:
    void onOrderPlaced(bool isBuy, uint32_t level, gateway::PlaceOrderOutcome outcome);
    void onOrderCompleted(std::string orderId);
    void onOrderChanged(orderbook::OwnOrder ownOrder);

private:
    Promise<void> initializeGrid();
    Promise<void> placeOrder(bool isBuy, uint32_t level);
    Promise<void> cancelOrder(std::string localId);

private:
    Config _cfg;
    std::unique_ptr<gateway::ExchangeGateway> _gateway;

    // orderId -> { level, orderbook::OwnOrder }
    std::map<std::string, std::pair<uint32_t, orderbook::OwnOrder>> _orders;
};
}

#endif // MOVINGGRIDSTRATEGY_HPP
