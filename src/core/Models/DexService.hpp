#ifndef DEXSERVICE_HPP
#define DEXSERVICE_HPP

#include <QObject>
#include <SwapService.hpp>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>

namespace swaps {
class SwapService;
class RefundableFeeManager;
}

namespace orderbook {
class OrderbookClient;
class ChannelRentingManager;
}

class AssetsTransactionsCache;
class PaymentNodesManager;
class WalletAssetsModel;
class TradingModelBatchedDataSource;
class DexSwapClientFactory;
class WalletDexStateModel;
class TradingBotService;

class DexService : public QObject {
    Q_OBJECT
public:
    struct TraidingPair {
        using BalanceInterval = std::pair<Balance, Balance>;

        QString pairId;
        BalanceInterval buyFundsInterval;
        BalanceInterval buyPriceInterval;
        BalanceInterval sellFundsInterval;
        BalanceInterval sellPriceInterval;
        double buyFeePercent;
        double sellFeePercent;
    };

    using TradingPairs = std::vector<TraidingPair>;

    explicit DexService(const WalletAssetsModel& assetsModel,
        PaymentNodesManager& paymentNodesManager, AssetsTransactionsCache& txCache,
        QObject* parent = nullptr);
    ~DexService();

    void start(QString clientPubKey);
    void stop();

    bool isStarted() const;
    bool isConnected() const;
    bool isInMaintenance() const;

    static bool IsStaging();

    TradingModelBatchedDataSource& ordersDataSource() const;
    swaps::SwapService& swapService() const;
    swaps::RefundableFeeManager* feeRefunding() const;
    orderbook::OrderbookClient* orderbook() const;
    orderbook::ChannelRentingManager* channelRentingManager() const;

    Promise<void> activatePair(QString pairId);
    Promise<void> deactivateActivePair();
    Promise<bool> verifyHubRoute(AssetID assetID, Balance amount, bool selfIsSource);
    Promise<void> addCurrencies(std::vector<AssetID> assetIDs);
    const TraidingPair& currentActivatedPair() const;
    const TradingPairs& tradingPairs() const;
    WalletDexStateModel& stateModel() const;
    TradingBotService& tradingBot() const;

signals:
    void started();
    void stopped();
    void isConnectedChanged(bool connected);
    void tradingPairsChanged();
    void lndsChanged(std::optional<std::pair<AssetID, AssetID>> swapAssetsPair);
    void isInMaintenanceChanged(bool isInMaintenance);

private slots:
    void onStateChanged(swaps::SwapService::State state);
    void onLightningBalanceChanged();

private:
    void init(QString clientPubKey);
    void setInMaintenance(bool inMaintenance);

private:
    const WalletAssetsModel& _assetsModel;
    PaymentNodesManager& _paymentNodesManager;
    AssetsTransactionsCache& _txCache;
    QPointer<WalletDexStateModel> _stateModel;

    std::unique_ptr<DexSwapClientFactory> _dexFactory;

    Utils::WorkerThread _dexWorker;
    qobject_delete_later_unique_ptr<swaps::SwapService> _swapService;
    qobject_delete_later_unique_ptr<TradingBotService> _tradingBot;

    Utils::WorkerThread _orderBatchingWorker;
    qobject_delete_later_unique_ptr<TradingModelBatchedDataSource> _batchedDataSource;

    TradingPairs _tradingPairs;
    TraidingPair _currentActivatedPair;
    bool _isConnected{ false };
    bool _isInMaintenance{ false };
};

#endif // DEXSERVICE_HPP
