#ifndef TRADINGBOTSERVICE_HPP
#define TRADINGBOTSERVICE_HPP

#include <QObject>
#include <memory>

#include <Utils/Utils.hpp>

namespace tradingbot {
class TradingBot;
}

class DexService;

/*!
 * \brief The TradingBotService is a wrapper over tradingbot::TradingBot which provides concurrent
 * access and utility functions to interact with trading bot.
 */
class TradingBotService : public QObject {
    Q_OBJECT
public:
    explicit TradingBotService(DexService& dexService, QObject* parent = nullptr);
    ~TradingBotService();

    Promise<void> start(
        QString pairId, uint32_t risk, uint32_t baseQuantity, uint32_t quoteQuantity);
    Promise<void> stop();
    uint32_t gridLevels() const;

private:
    std::unique_ptr<tradingbot::TradingBot> _tradingBot;
};

#endif // TRADINGBOTSERVICE_HPP
