#ifndef APPLICATIONVIEWMODEL_HPP
#define APPLICATIONVIEWMODEL_HPP

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
class LnDaemonsManager;
class PaymentNodeStateManager;
class AssetsBalance;
class LocalCurrency;
class WalletViewModel;
class Wallet;
class SyncService;
class AbstractChainSyncManagerFactory;
class AbstractNetworkingFactory;
class AssetsTransactionsCache;
class AbstractChainManager;
class AbstractChainDataSource;
class AbstractMutableChainManager;
class AssetsRemotePriceModel;
class LocalCurrencyViewModel;
class DexService;
class GRPCServer;
class Context;
class ApplicationPaymentNodesViewModel;
class NotificationData;
class DexStatusManager;
class ChannelRentalHelper;
class WalletMarketSwapModel;
class PaymentNodesManager;
class LockingViewModel;
class TradingBotModel;

class ApplicationViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(LocalCurrencyViewModel* localCurrencyViewModel READ localCurrencyViewModel CONSTANT)
    Q_PROPERTY(QObject* emulator READ emulatorViewModel CONSTANT)
    Q_PROPERTY(WalletViewModel* walletViewModel READ walletViewModel CONSTANT)
    Q_PROPERTY(ApplicationPaymentNodesViewModel* paymentNodeViewModel READ applicationPaymentNodeViewModel CONSTANT)
    Q_PROPERTY(QObject* syncService READ syncService CONSTANT)
    Q_PROPERTY(bool emulated READ isEmulated CONSTANT)

    Q_PROPERTY(QString clientId READ clientID CONSTANT)
    Q_PROPERTY(LockingViewModel* lockingViewModel READ lockingViewModel CONSTANT)
    Q_PROPERTY(TradingBotModel* tradingBotModel READ tradingBotModel CONSTANT)

public:
    explicit ApplicationViewModel(QObject* parent = nullptr);
    virtual ~ApplicationViewModel();

    void init();

    AbstractChainManager* chainManager() const;
    WalletDataSource* dataSource() const;
    AccountDataSource* accountDataSource() const;
    AssetsTransactionsCache* transactionsCache() const;
    WalletAssetsModel* assetsModel() const;
    AssetsBalance* assetsBalance() const;
    LocalCurrency* localCurrency() const;
    WalletViewModel* walletViewModel() const;
    QObject* syncService() const;
    PaymentNodesManager* paymentNodesManager() const;
    PaymentNodeStateManager* paymentNodeStateManager() const;
    AssetsRemotePriceModel* currencyRates() const;
    bool isEmulated() const;
    LocalCurrencyViewModel* localCurrencyViewModel() const;
    AbstractNetworkingFactory* apiClientsFactory() const;
    QObject* emulatorViewModel() const;
    DexService* dexService() const;
    ApplicationPaymentNodesViewModel *applicationPaymentNodeViewModel() const;
    NotificationData* notificationData() const;
    AbstractChainDataSource* chainDataSource() const;
    DexStatusManager* dexStatusManager() const;
    ChannelRentalHelper* channelRentalHelper() const;
    QString clientID() const;
    WalletMarketSwapModel* marketSwapModel() const;
    LockingViewModel* lockingViewModel() const;
    TradingBotModel* tradingBotModel() const;

    static ApplicationViewModel* Instance();
    static bool IsEmulated();
    static bool IsMobile();
    static void UseStagingEnv(bool staging);
    static bool IsStagingEnv();

    void destroyContext();

public slots:
    void loadContext();

signals:
    void allTransactionChanged();
    void loadContextFinished();
    void destroyContextFinished();

private:
    std::unique_ptr<WalletAssetsModel> _assetsModel;
    std::unique_ptr<Context> _ctx;
};

#endif // APPLICATIONVIEWMODEL_HPP
