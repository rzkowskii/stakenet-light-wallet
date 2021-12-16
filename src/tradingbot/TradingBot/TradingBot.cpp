#include "TradingBot.hpp"
#include <TradingBot/ExchangeGatewayFactory.hpp>
#include <TradingBot/MovingGridStrategy.hpp>

namespace tradingbot {

static const double DEFAULT_GRID_INTERVAL = 0.1;
static const double DEFAULT_ORDER_FEE = 0.1;

//==============================================================================

TradingBot::TradingBot(std::unique_ptr<gateway::ExchangeGatewayFactory> factory, QObject* parent)
    : QObject(parent)
    , _factory(std::move(factory))
{
}

//==============================================================================

TradingBot::~TradingBot() {}

//==============================================================================

Promise<void> TradingBot::start(TradingBotConfig cfg)
{
    auto basePromise = cleanupStrategy(std::move(_strategy));
    auto exchangeGateway = _factory->createGateway();

    MovingGridStrategy::Config strategyConfig;
    strategyConfig.pairId = cfg.pairId;
    strategyConfig.levels = DEFAULT_GRID_LEVELS;
    strategyConfig.interval = DEFAULT_GRID_INTERVAL;
    strategyConfig.risk = cfg.risk;
    strategyConfig.orderFee = DEFAULT_ORDER_FEE;
    strategyConfig.baseQuantity = cfg.baseQuantity;
    strategyConfig.quoteQuantity = cfg.quoteQuantity;
    _strategy.reset(new MovingGridStrategy(strategyConfig, std::move(exchangeGateway)));

    return basePromise.then([this] {
        // initialize grid with initial state
        return _strategy->start();
    });
}

//==============================================================================

Promise<void> TradingBot::stop()
{
    return cleanupStrategy(std::move(_strategy)).then([this]() { _strategy.reset(); });
}

//==============================================================================

void TradingBot::timerEvent(QTimerEvent* event){ Q_UNUSED(event) }

//==============================================================================

Promise<void> TradingBot::cleanupStrategy(StrategyRef strategy) const
{
    if (!strategy) {
        return QtPromise::resolve();
    }

    return strategy->stop();
}

//==============================================================================
}
