#include "TradingBotModel.hpp"
#include <Data/TradingBotService.hpp>
#include <Models/DexService.hpp>
#include <Models/WalletDexStateModel.hpp>
#include <Utils/Logging.hpp>
#include <Utils/Utils.hpp>

//==============================================================================

TradingBotModel::TradingBotModel(const DexService& dexService, QObject* parent)
    : QObject(parent)
    , _dexService(dexService)
{
}

//==============================================================================

bool TradingBotModel::isActive() const
{
    return _isActive;
}

//==============================================================================

void TradingBotModel::start(int risk, int baseQuantity, int quoteQuantity)
{
    if (_dexService.stateModel().hasSwapPair()) {
        auto currentActivatedPairId = _dexService.currentActivatedPair().pairId;
        _dexService.tradingBot()
            .start(currentActivatedPairId, risk, baseQuantity, quoteQuantity)
            .then([this, currentActivatedPairId]() {
                setActivity(true);
                LogCDebug(TradingBot) << "Bot started";
            })
            .fail([](const std::exception& ex) {
                LogCCritical(TradingBot) << "Bot start failed" << ex.what();
            })
            .fail([]() { LogCCritical(TradingBot) << "Bot start failed"; });
    }
}

//==============================================================================

void TradingBotModel::stop()
{
    if (_dexService.stateModel().hasSwapPair()) {
        _dexService.tradingBot()
            .stop()
            .then([this]() {
                setActivity(false);
                LogCDebug(TradingBot) << "Bot stopped";
            })
            .fail([](const std::exception& ex) {
                LogCCritical(TradingBot) << "Bot stop failed" << ex.what();
            })
            .fail([]() { LogCCritical(TradingBot) << "Bot stop failed"; });
    }
}

//==============================================================================

int TradingBotModel::gridLevels()
{
    return _dexService.tradingBot().gridLevels();
}

//==============================================================================

void TradingBotModel::setActivity(bool value)
{
    if (_isActive != value) {
        _isActive = value;
        activityChanged();
    }
}

//==============================================================================
