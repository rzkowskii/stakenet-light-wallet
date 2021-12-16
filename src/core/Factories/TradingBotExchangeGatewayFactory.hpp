#ifndef TRADINGBOTEXCHANGEGATEWAYFACTORY_HPP
#define TRADINGBOTEXCHANGEGATEWAYFACTORY_HPP

#include <QObject>
#include <TradingBot/ExchangeGatewayFactory.hpp>

//==============================================================================

class DexService;

//==============================================================================

class TradingBotExchangeGatewayFactory : public QObject,
                                         public tradingbot::gateway::ExchangeGatewayFactory {
    Q_OBJECT
public:
    explicit TradingBotExchangeGatewayFactory(DexService& dexService, QObject* parent = nullptr);

    // ExchangeGatewayFactory interface
public:
    tradingbot::gateway::ExchangeGatewayRef createGateway() override;

private:
    DexService& _dexService;
};

//==============================================================================

#endif // TRADINGBOTEXCHANGEGATEWAYFACTORY_HPP
