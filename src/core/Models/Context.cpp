#include "Context.hpp"
#include <Chain/AbstractAssetAccount.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/AccountCacheImpl.hpp>
#include <Chain/CachedChainDataSource.hpp>
#include <Chain/ChainManager.hpp>
#include <Chain/ChainSyncHelper.hpp>
#include <Chain/ChainSyncManager.hpp>
#include <Chain/RegtestChain.hpp>
#include <Chain/TransactionsCache.hpp>
#include <Data/AssetsBalance.hpp>
#include <Data/LocalCurrency.hpp>
#include <Data/NotificationData.hpp>
#include <Data/Wallet.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/ApiClientNetworkingFactory.hpp>
#include <Factories/AssetBalanceProviderFactory.hpp>
#include <Factories/ChainSyncManagerFactory.hpp>
#include <Factories/EmulatorChainSyncManagerFactory.hpp>
#include <Models/AllOnChainTxDataSource.hpp>
#include <Models/AssetsRemotePriceModel.hpp>
#include <Models/ChannelRentalHelper.hpp>
#include <Models/DaemonSlotsManager.hpp>
#include <Models/DexService.hpp>
#include <Models/DexStatusManager.hpp>
#include <Models/EmulatorWalletDataSource.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodeStateManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Models/SyncService.hpp>
#include <Models/TradingBotModel.hpp>
#include <Models/WalletMarketSwapModel.hpp>
#include <Models/WalletTransactionsListModel.hpp>
#include <Networking/NetworkConnectionState.hpp>
#include <Networking/XSNBlockExplorerHttpClient.hpp>
#include <RPC/GRPCServer.hpp>
#include <Tools/Common.hpp>
#include <Utils/GenericProtoDatabase.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationPaymentNodesViewModel.hpp>
#include <ViewModels/ApplicationViewModel.hpp>
#include <ViewModels/EmulatorViewModel.hpp>
#include <ViewModels/LocalCurrencyViewModel.hpp>
#include <ViewModels/LockingViewModel.hpp>
#include <ViewModels/WalletViewModel.hpp>

#include <QDir>
#include <QStandardPaths>
#include <key_io.h>
#include <script/standard.h>

static const bool DEFAULT_USE_REAL_WALLET = false;
static std::vector<AssetID> loadingChains;

//==============================================================================

Context::Context(WalletAssetsModel* assetsModel, QObject* parent)
    : QObject(parent)
{
    _walletAssetsModel = assetsModel;
    init();
}

//==============================================================================

Context::~Context() {}

//==============================================================================

AbstractChainManager* Context::chainManager() const
{
    return _chainManager.get();
}

//==============================================================================

WalletDataSource* Context::walletDataSource() const
{
    return _walletDataSource.get();
}

//==============================================================================

AccountDataSource* Context::accountDataSource() const
{
    return _accountDataSource;
}

//==============================================================================

AssetsTransactionsCache* Context::transactionsCache() const
{
    return _transactionsCache.get();
}

//==============================================================================

AssetsAccountsCache* Context::accountsCache() const
{
    return _accountsCache.get();
}

//==============================================================================

AssetsBalance* Context::assetsBalance() const
{
    return _assetsBalance.get();
}

//==============================================================================

LocalCurrency* Context::localCurrency() const
{
    return _localCurrency.get();
}

//==============================================================================

WalletViewModel* Context::walletViewModel() const
{
    return _walletViewModel.get();
}

//==============================================================================

SyncService* Context::syncService() const
{
    return _syncService.get();
}

//==============================================================================

AssetsRemotePriceModel* Context::currencyRates() const
{
    return _currencyRates.get();
}

//==============================================================================

LocalCurrencyViewModel* Context::localCurrencyViewModel() const
{
    return _localCurrencyViewModel.get();
}

//==============================================================================

AbstractNetworkingFactory* Context::apiClientsFactory() const
{
    return _apiClientsFactory.get();
}

//==============================================================================

EmulatorViewModel* Context::emulatorViewModel() const
{
    return _emulatorViewModel.get();
}

//==============================================================================

AbstractChainDataSource* Context::chainDataSource() const
{
    return _chainDataSource.get();
}

//==============================================================================

GRPCServer* Context::grpcServer() const
{
    return _grpcServer.get();
}

//==============================================================================

PaymentNodeStateManager* Context::paymentNodeStateManager() const
{
    return _paymentNodeStateManager.get();
}

//==============================================================================

DexService* Context::dexService() const
{
    return _dexService.get();
}

//==============================================================================

ApplicationPaymentNodesViewModel* Context::applicationPayNodeViewModel() const
{
    return _applicationPayNodeViewModel.get();
}

//==============================================================================

NotificationData* Context::notificationData() const
{
    return _notificationData.get();
}

//==============================================================================

DexStatusManager* Context::dexStatusManager() const
{
    return _dexStatusManager.get();
}

//==============================================================================

ChannelRentalHelper* Context::channelRentalHelper() const
{
    return _channelRentalHelper.get();
}

//==============================================================================

WalletMarketSwapModel* Context::marketSwapModel() const
{
    return _marketSwapModel.get();
}

//==============================================================================

PaymentNodesManager* Context::paymentNodesManager() const
{
    return _paymentNodesManager.get();
}

//==============================================================================

LockingViewModel* Context::lockingViewModel() const
{
    return _lockingViewModel.get();
}

//==============================================================================

TradingBotModel* Context::tradingBotModel() const
{
    return _tradingBotModel.get();
}

//==============================================================================

void Context::init()
{
    LogCDebug(General) << "Initing with data dir:"
                       << GetDataDir(ApplicationViewModel::IsEmulated());

    try {

        auto db = std::make_shared<Utils::LevelDBSharedDatabase>(
            GetTxDBPath(ApplicationViewModel::IsEmulated()).toStdString(), 10000);

        initNetworkingState();
        initWorkerThread();
        initTransactionsCache(db);
        initAccountsCache(db);
        initNetworkingFactories();
        initCurrencyRates();
        initChainManager();
        initDataSources();
        initFactories();
        initSyncService();
        initWalletViewModel();
        initPaymentNodesManager();
        initPaymentNodesProcessManagers();

        initDexService();
        initTradingBotModel();
        initWalletMarketSwapModel();
        initWalletAssetsBalance();
        initPaymentNodeStateManager();
        initDexStatusManager();
        initChannelRentalHelper();
        initRPCServer();
        initApplicationPayNodeViewModel();
        initNotificationData();
        initLockingViewModel();

    } catch (std::exception& ex) {
        LogCDebug(General) << "Critical init error:" << ex.what();
        throw ex;
    }
}

//==============================================================================

void Context::initDataSources()
{
    Q_ASSERT(_workerThread);
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_chainManager);
    Q_ASSERT(_transactionsCache);

    if (ApplicationViewModel::IsEmulated() && !DEFAULT_USE_REAL_WALLET) {
        qobject_delete_later_unique_ptr<EmulatorWalletDataSource> wallet(
            new EmulatorWalletDataSource(*_walletAssetsModel, *_transactionsCache));
        _filterMatcher = wallet.get();
        _walletDataSource = std::move(wallet);
    } else {
        qobject_delete_later_unique_ptr<Wallet> wallet(new Wallet(*_walletAssetsModel,
            *_transactionsCache, *_chainManager, ApplicationViewModel::IsEmulated()));
        _filterMatcher = wallet.get();
        _utxoDataSource = wallet.get();
        _accountDataSource = wallet.get();
        _walletDataSource = std::move(wallet);
    }

    if (ApplicationViewModel::IsEmulated()) {
        _emulatorViewModel.reset(new EmulatorViewModel);
    }

    _walletDataSource->moveToThread(_workerThread.get());
}

//==============================================================================

void Context::initWalletAssetsBalance()
{
    Q_ASSERT(transactionsCache());
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_walletViewModel);
    Q_ASSERT(_chainManager);
    Q_ASSERT(accountsCache());
    Q_ASSERT(_paymentNodesManager);
    std::unique_ptr<AbstractAssetBalanceProviderFactory> factory
        = std::make_unique<AssetBalanceProviderFactory>(_transactionsCache.get(),
            _accountsCache.get(), _chainManager.get(), _paymentNodesManager.get());
    _assetsBalance.reset(new AssetsBalance(_walletAssetsModel, _localCurrencyViewModel.get(),
        _chainManager.get(), std::move(factory)));
    connect(_walletViewModel.get(), &WalletViewModel::transactionsDatabaseLoaded,
        _assetsBalance.get(), &AssetsBalance::update);
}

//==============================================================================

void Context::initTransactionsCache(std::shared_ptr<Utils::LevelDBSharedDatabase> provider)
{
    //    auto db = std::make_shared<Utils::LevelDBSharedDatabase>(
    //        GetTxDBPath(ApplicationViewModel::IsEmulated()).toStdString(), 10000);
    _transactionsCache.reset(new TransactionsCacheImpl(provider));
    _transactionsCache->moveToThread(_workerThread.get());

    _accountsCache.reset();
}

//==============================================================================

void Context::initAccountsCache(std::shared_ptr<Utils::LevelDBSharedDatabase> provider)
{
    Q_ASSERT(_walletAssetsModel);

    _accountsCache.reset(new AccountCacheImpl(provider, *_walletAssetsModel));
    _accountsCache->moveToThread(_workerThread.get());
}

//==============================================================================

void Context::initWalletViewModel()
{
    Q_ASSERT(walletDataSource());
    Q_ASSERT(_apiClientsFactory);
    Q_ASSERT(_transactionsCache);
    Q_ASSERT(_syncService);
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_chainManager);

    _walletViewModel.reset(
        new WalletViewModel(*walletDataSource(), *_utxoDataSource, *_transactionsCache,
            *_accountsCache, *_walletAssetsModel, *_chainManager, *_syncService, this));
}

//==============================================================================

void Context::initPaymentNodesProcessManagers()
{
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_workerThread);
    Q_ASSERT(_transactionsCache);
    Q_ASSERT(_paymentNodesManager);

    _paymentNodeDaemonsManager.reset(
        new DaemonSlotsManager(*_walletAssetsModel, *_paymentNodesManager));
    connect(_walletViewModel.get(), &WalletViewModel::walletLoaded, this, [this]() {
        _paymentNodeDaemonsManager->start(
            [this](auto assetID) { return _syncService->isChainSynced(assetID); });
    });
}

//==============================================================================

void Context::initSyncService()
{
    Q_ASSERT(_chainManager);
    Q_ASSERT(_syncManagersFactory);
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_walletDataSource);
    Q_ASSERT(_workerThread);
    _syncService.reset(new SyncService(
        *_syncManagersFactory, *_chainManager, *_walletAssetsModel, *_walletDataSource));
}

//==============================================================================

void Context::initNetworkingFactories()
{
    _apiClientsFactory.reset(new ApiClientNetworkingFactory);
    _apiClientsFactory->moveToThread(_workerThread.get());
}

//==============================================================================

void Context::initFactories()
{
    Q_ASSERT(walletDataSource());
    Q_ASSERT(transactionsCache());
    Q_ASSERT(_filterMatcher);
    Q_ASSERT(_workerThread);
    Q_ASSERT(_chainDataSource);

    if (ApplicationViewModel::IsEmulated()) {
        Q_ASSERT(_emulatorViewModel);
        _syncManagersFactory.reset(new EmulatorChainSyncManagerFactory(_walletAssetsModel,
            _apiClientsFactory.get(), *_workerThread, *walletDataSource(), *transactionsCache(),
            *_filterMatcher, *_emulatorViewModel));
    } else {
        _syncManagersFactory.reset(new ChainSyncManagerFactory(_walletAssetsModel,
            _apiClientsFactory.get(), *_workerThread, *walletDataSource(), *accountDataSource(),
            *_chainDataSource, *transactionsCache(), *accountsCache(), *_filterMatcher));
    }
}

//==============================================================================

void Context::initChainManager()
{
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_workerThread);

    if (ApplicationViewModel::IsEmulated()) {
        qobject_delete_later_unique_ptr<RegtestDataSource> regtestChain(
            new RegtestDataSource(*_walletAssetsModel, this));

        auto regtestChainRaw = regtestChain.get();

        connect(_emulatorViewModel.get(), &EmulatorViewModel::newBlockRequested, this,
            [regtestChainRaw, this](AssetID assetID, int count, QString addressTo) {
                const auto& params = _walletAssetsModel->assetById(assetID).params().params;
                auto script = bitcoin::GetScriptForDestination(
                    bitcoin::DecodeDestination(addressTo.toStdString(), params));
                regtestChainRaw->chain(assetID).generateBlocks(script, count);
            });

        connect(_emulatorViewModel.get(), &EmulatorViewModel::reorgRequested, this,
            [regtestChainRaw, this](
                AssetID assetID, int disconnectCount, int connectCount, QString addressTo) {
                const auto& params = _walletAssetsModel->assetById(assetID).params().params;
                auto script = bitcoin::GetScriptForDestination(
                    bitcoin::DecodeDestination(addressTo.toStdString(), params));
                regtestChainRaw->chain(assetID).reorganizeChain(
                    script, disconnectCount, connectCount);
            });

        qobject_delete_later_unique_ptr<RegtestChainManager> chainManager(
            new RegtestChainManager(*regtestChain, *_walletAssetsModel));

        _chainManager = std::move(chainManager);
        _chainDataSource = std::move(regtestChain);
        _utxoDataSource = regtestChainRaw;
    } else {
        Q_ASSERT(_apiClientsFactory);
        Q_ASSERT(_transactionsCache);
        _chainManager.reset(new ChainManager(
            *_walletAssetsModel, GetChainIndexDir(ApplicationViewModel::IsEmulated())));
        _chainManager->moveToThread(_workerThread.get());

        Q_ASSERT(_chainManager);
        _chainDataSource.reset(new CachedChainDataSource(
            *_apiClientsFactory, *_walletAssetsModel, *_transactionsCache, *_chainManager));
        _chainDataSource->moveToThread(_workerThread.get());
    }
}

//==============================================================================

void Context::initWorkerThread()
{
    _workerThread.reset(new Utils::WorkerThread);
    _workerThread->rename("Stakenet-MainWorker");
    _workerThread->start();
}

//==============================================================================

void Context::initCurrencyRates()
{
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_apiClientsFactory);

    _localCurrency.reset(new LocalCurrency);
    _currencyRates.reset(new AssetsRemotePriceModel(
        *_walletAssetsModel, _apiClientsFactory->createRemotePriceProvider(), *_localCurrency));
    _localCurrencyViewModel.reset(new LocalCurrencyViewModel(*_currencyRates, *_localCurrency));
}

//==============================================================================

void Context::initRPCServer()
{
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_chainManager);
    Q_ASSERT(_workerThread);
    Q_ASSERT(_apiClientsFactory);
    Q_ASSERT(_transactionsCache);
    Q_ASSERT(_walletDataSource);
    Q_ASSERT(_utxoDataSource);

    _grpcServer.reset(new GRPCServer(*_walletAssetsModel, *_chainManager, *_chainDataSource,
        *_transactionsCache, *_walletDataSource, _utxoDataSource));
    _grpcServer->moveToThread(_workerThread.get());

    connect(_walletViewModel.get(), &WalletViewModel::transactionsDatabaseLoaded, _grpcServer.get(),
        &GRPCServer::start);
}

//==============================================================================

void Context::initDexService()
{
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_paymentNodesManager);
    Q_ASSERT(_transactionsCache);

    _dexService.reset(
        new DexService(*_walletAssetsModel, *_paymentNodesManager, *_transactionsCache));
    QtPromise::connect(_paymentNodeDaemonsManager.get(), &DaemonSlotsManager::started).then([this] {
        Q_ASSERT(_walletDataSource->isLoaded());
        _dexService->start(_walletDataSource->identityPubKey());
    });
}

//==============================================================================

void Context::initApplicationPayNodeViewModel()
{
    Q_ASSERT(_paymentNodeDaemonsManager);
    Q_ASSERT(_dexService);
    _applicationPayNodeViewModel.reset(
        new ApplicationPaymentNodesViewModel(*_paymentNodeDaemonsManager, *_dexService));
}

//==============================================================================

void Context::initNotificationData()
{
    Q_ASSERT(_walletAssetsModel);
    _notificationData.reset(new NotificationData(*_walletAssetsModel));
}

//==============================================================================

void Context::initPaymentNodeStateManager()
{
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_assetsBalance);
    Q_ASSERT(_paymentNodesManager);
    _paymentNodeStateManager.reset(
        new PaymentNodeStateManager(*_walletAssetsModel, *_assetsBalance, *_paymentNodesManager));
}

//==============================================================================

void Context::initDexStatusManager()
{
    Q_ASSERT(_paymentNodeStateManager);
    Q_ASSERT(_walletAssetsModel);
    _dexStatusManager.reset(
        new DexStatusManager(_paymentNodeStateManager.get(), _walletAssetsModel));
}

//==============================================================================

void Context::initChannelRentalHelper()
{
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_dexService);
    _channelRentalHelper.reset(new ChannelRentalHelper(*_dexService, *_walletAssetsModel));
    connect(_dexService.get(), &DexService::started, _channelRentalHelper.get(),
        &ChannelRentalHelper::start);
}

//==============================================================================

void Context::initNetworkingState()
{
    _networkConnectionThread.reset(new Utils::WorkerThread, [](Utils::WorkerThread* obj) {
        NetworkConnectionState::Shutdown();
        obj->quit();
        obj->wait();
        delete obj;
    });
    NetworkConnectionState::Initialize();
    NetworkConnectionState::Singleton()->moveToThread(_networkConnectionThread.get());
    _networkConnectionThread->start();
}

//==============================================================================

void Context::initWalletMarketSwapModel()
{
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_paymentNodesManager);
    Q_ASSERT(_dexService);
    _marketSwapModel.reset(
        new WalletMarketSwapModel(*_walletAssetsModel, *_dexService, *_paymentNodesManager));
}

//==============================================================================

void Context::initPaymentNodesManager()
{
    Q_ASSERT(_transactionsCache);
    Q_ASSERT(_walletAssetsModel);
    Q_ASSERT(_workerThread);
    Q_ASSERT(_accountDataSource);
    Q_ASSERT(_apiClientsFactory);

    _paymentNodesManager.reset(new PaymentNodesManager(_workerThread.get(), transactionsCache(),
        _walletAssetsModel, *_accountDataSource, *_apiClientsFactory));
}

//==============================================================================

void Context::initLockingViewModel()
{
    _lockingViewModel.reset(new LockingViewModel(*walletDataSource()));
}

//==============================================================================

void Context::initTradingBotModel()
{
    Q_ASSERT(_dexService);
    _tradingBotModel.reset(new TradingBotModel(*_dexService));
}

//==============================================================================
