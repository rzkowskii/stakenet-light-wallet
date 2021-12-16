#include "TradingBotService.hpp"
#include <Factories/TradingBotExchangeGatewayFactory.hpp>
#include <TradingBot/TradingBot.hpp>
#include <Utils/Logging.hpp>

//==============================================================================

TradingBotService::TradingBotService(DexService& dexService, QObject* parent)
    : QObject(parent)
{
    _tradingBot.reset(
        new tradingbot::TradingBot(std::make_unique<TradingBotExchangeGatewayFactory>(dexService)));
}

//==============================================================================

TradingBotService::~TradingBotService() {}

//==============================================================================

Promise<void> TradingBotService::start(
    QString pairId, uint32_t risk, uint32_t baseQuantity, uint32_t quoteQuantity)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        return QMetaObject::invokeMethod(this, [=] {
            LogCDebug(TradingBot) << "Starting trading bot with params"
                                  << "pairId:" << pairId << "risk:" << risk
                                  << "baseQuantity:" << baseQuantity
                                  << "quoteQuantity:" << quoteQuantity;

            tradingbot::TradingBotConfig cfg;
            cfg.pairId = pairId;
            cfg.risk = risk;
            cfg.baseQuantity = baseQuantity;
            cfg.quoteQuantity = quoteQuantity;
            _tradingBot->start(cfg).then([this, resolve] { resolve(); }).fail([reject] {
                reject(std::current_exception());
            });
        });
    });
}

//==============================================================================

Promise<void> TradingBotService::stop()
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        return QMetaObject::invokeMethod(this, [=] {
            LogCDebug(TradingBot) << "Stopting trading bot";
            _tradingBot->stop().then([this, resolve] { resolve(); }).fail([reject] {
                reject(std::current_exception());
            });
        });
    });
}

//==============================================================================

uint32_t TradingBotService::gridLevels() const
{
    return tradingbot::DEFAULT_GRID_LEVELS;
}

//==============================================================================
