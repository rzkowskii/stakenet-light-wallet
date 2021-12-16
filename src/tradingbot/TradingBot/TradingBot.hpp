#ifndef TRADINGBOT_HPP
#define TRADINGBOT_HPP

#include <QObject>
#include <Utils/Utils.hpp>

namespace tradingbot {

namespace gateway {
    struct ExchangeGatewayFactory;
}

class MovingGridStrategy;

static const uint32_t DEFAULT_GRID_LEVELS = 5;

//==============================================================================

struct TradingBotConfig {
    QString pairId;
    uint32_t risk;
    uint32_t baseQuantity;
    uint32_t quoteQuantity;
};

//==============================================================================

class TradingBot : public QObject {
    Q_OBJECT
public:
    using StrategyRef = qobject_delete_later_unique_ptr<MovingGridStrategy>;
    explicit TradingBot(
        std::unique_ptr<gateway::ExchangeGatewayFactory> factory, QObject* parent = nullptr);
    ~TradingBot();

    Promise<void> start(TradingBotConfig cfg);
    Promise<void> stop();

private:
    void timerEvent(QTimerEvent* event) override;
    Promise<void> cleanupStrategy(StrategyRef strategy) const;

private:
    std::unique_ptr<gateway::ExchangeGatewayFactory> _factory;
    StrategyRef _strategy;
    TradingBotConfig _cfg;
};

//==============================================================================
}

#endif // TRADINGBOT_HPP
