#include "TradingBotExchangeGatewayFactory.hpp"
#include <Data/TradingBotExchangeGateway.hpp>

//==============================================================================

TradingBotExchangeGatewayFactory::TradingBotExchangeGatewayFactory(
    DexService& dexService, QObject* parent)
    : QObject{ parent }
    , _dexService(dexService)
{
}

//==============================================================================

tradingbot::gateway::ExchangeGatewayRef TradingBotExchangeGatewayFactory::createGateway()
{
    return std::make_unique<TradingBotExchangeGateway>(_dexService);
}

//==============================================================================
