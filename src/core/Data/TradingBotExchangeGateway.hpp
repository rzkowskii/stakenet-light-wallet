#ifndef TRADINGBOTEXCHANGEGATEWAY_HPP
#define TRADINGBOTEXCHANGEGATEWAY_HPP

#include <TradingBot/ExchangeGateway.hpp>

class DexService;

class TradingBotExchangeGateway : public tradingbot::gateway::ExchangeGateway {
public:
    explicit TradingBotExchangeGateway(const DexService& dexService, QObject* parent = nullptr);

    // ExchangeGateway interface
public:
    double getLastPrice(QString pairId) const override;
    Promise<tradingbot::gateway::PlaceOrderOutcome> placeOrder(
        orderbook::LimitOrder order) override;
    boost::optional<orderbook::OwnOrder> tryGetOwnOrder(
        std::string pairId, std::string orderId) const override;
    virtual Promise<void> cancelOrder(std::string pairId, std::string localId) const override;

private:
    const DexService& _dexService; // used to query state of dex service
};

#endif // TRADINGBOTEXCHANGEGATEWAY_HPP
