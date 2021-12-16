#ifndef CONTEXT_HPP
#define CONTEXT_HPP

#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>

struct BlockFilterMatchable;
class EmulatorWalletDataSource;
class EmulatorViewModel;
class WalletDataSource;
class AccountDataSource;
class WalletAssetsModel;
class WalletTransactionsListModel;
class AssetsBalance;
class PaymentNodeStateManager;
class LocalCurrency;
class WalletViewModel;
class Wallet;
class SyncService;
class AbstractChainSyncManagerFactory;
class AbstractNetworkingFactory;
class AssetsTransactionsCache;
class AssetsAccountsCache;
class AbstractChainManager;
class AbstractChainDataSource;
class AbstractMutableChainManager;
class AssetsRemotePriceModel;
class UTXOSetDataSource;
class LocalCurrencyViewModel;
class GRPCServer;
class DexService;
class ApplicationPaymentNodesViewModel;
class NotificationData;
class ChannelRentalManager;
class DexStatusManager;
class ChannelRentalHelper;
class WalletMarketSwapModel;
class LockingViewModel;
class DaemonSlotsManager;
class PaymentNodesManager;
class TradingBotModel;

namespace Utils {
class LevelDBSharedDatabase;
}

class Context : public QObject {
public:
    explicit Context(WalletAssetsModel* assetsModel, QObject* parent = nullptr);
    ~Context();
    AbstractChainManager* chainManager() const;
    WalletDataSource* walletDataSource() const;
    AccountDataSource* accountDataSource() const;
    AssetsTransactionsCache* transactionsCache() const;
    AssetsAccountsCache* accountsCache() const;
    AssetsBalance* assetsBalance() const;
    LocalCurrency* localCurrency() const;
    WalletViewModel* walletViewModel() const;
    SyncService* syncService() const;
    AssetsRemotePriceModel* currencyRates() const;
    LocalCurrencyViewModel* localCurrencyViewModel() const;
    AbstractNetworkingFactory* apiClientsFactory() const;
    EmulatorViewModel* emulatorViewModel() const;
    AbstractChainDataSource* chainDataSource() const;
    GRPCServer* grpcServer() const;
    PaymentNodeStateManager* paymentNodeStateManager() const;
    DexService* dexService() const;
    ApplicationPaymentNodesViewModel* applicationPayNodeViewModel() const;
    NotificationData* notificationData() const;
    DexStatusManager* dexStatusManager() const;
    ChannelRentalHelper* channelRentalHelper() const;
    WalletMarketSwapModel* marketSwapModel() const;
    PaymentNodesManager* paymentNodesManager() const;
    LockingViewModel* lockingViewModel() const;
    TradingBotModel* tradingBotModel() const;

private:
    void init();
    void initDataSources();
    void initWalletAssetsBalance();
    void initTransactionsCache(std::shared_ptr<Utils::LevelDBSharedDatabase> provider);
    void initAccountsCache(std::shared_ptr<Utils::LevelDBSharedDatabase> provider);
    void initWalletViewModel();
    void initPaymentNodesProcessManagers();
    void initSyncService();
    void initNetworkingFactories();
    void initFactories();
    void initChainManager();
    void initWorkerThread();
    void initCurrencyRates();
    void initRPCServer();
    void initDexService();
    void initApplicationPayNodeViewModel();
    void initNotificationData();
    void initPaymentNodeStateManager();
    void initDexStatusManager();
    void initChannelRentalHelper();
    void initNetworkingState();
    void initWalletMarketSwapModel();
    void initPaymentNodesManager();
    void initLockingViewModel();
    void initTradingBotModel();

private:
    std::shared_ptr<Utils::WorkerThread> _networkConnectionThread;
    std::unique_ptr<Utils::WorkerThread> _workerThread;
    std::unique_ptr<LocalCurrency> _localCurrency;
    std::unique_ptr<AssetsRemotePriceModel> _currencyRates;
    std::unique_ptr<AssetsBalance> _assetsBalance;
    std::unique_ptr<EmulatorViewModel> _emulatorViewModel;
    std::unique_ptr<LocalCurrencyViewModel> _localCurrencyViewModel;
    qobject_delete_later_unique_ptr<AbstractNetworkingFactory> _apiClientsFactory;
    qobject_delete_later_unique_ptr<AssetsTransactionsCache> _transactionsCache;
    qobject_delete_later_unique_ptr<AssetsAccountsCache> _accountsCache;
    qobject_delete_later_unique_ptr<WalletDataSource> _walletDataSource;
    qobject_delete_later_unique_ptr<AbstractMutableChainManager> _chainManager;
    qobject_delete_later_unique_ptr<AbstractChainDataSource> _chainDataSource;
    std::unique_ptr<AbstractChainSyncManagerFactory> _syncManagersFactory;
    std::unique_ptr<WalletViewModel> _walletViewModel;
    std::unique_ptr<SyncService> _syncService;
    qobject_delete_later_unique_ptr<GRPCServer> _grpcServer;
    std::unique_ptr<DaemonSlotsManager> _paymentNodeDaemonsManager;
    std::unique_ptr<PaymentNodeStateManager> _paymentNodeStateManager;
    std::unique_ptr<DexService> _dexService;
    std::unique_ptr<ApplicationPaymentNodesViewModel> _applicationPayNodeViewModel;
    std::unique_ptr<NotificationData> _notificationData;
    std::unique_ptr<ChannelRentalHelper> _channelRentalHelper;
    std::unique_ptr<WalletMarketSwapModel> _marketSwapModel;
    std::unique_ptr<PaymentNodesManager> _paymentNodesManager;
    std::unique_ptr<LockingViewModel> _lockingViewModel;
    std::unique_ptr<TradingBotModel> _tradingBotModel;

    AccountDataSource* _accountDataSource{ nullptr };

    const BlockFilterMatchable* _filterMatcher{ nullptr };
    UTXOSetDataSource* _utxoDataSource{ nullptr };
    WalletAssetsModel* _walletAssetsModel{ nullptr };
    std::unique_ptr<DexStatusManager> _dexStatusManager;
};

#endif // CONTEXT_HPP
