#include "ApplicationViewModel.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
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
#include <Factories/ChainSyncManagerFactory.hpp>
#include <Factories/EmulatorChainSyncManagerFactory.hpp>
#include <Models/AllOnChainTxDataSource.hpp>
#include <Models/ChannelRentalHelper.hpp>
#include <Models/Context.hpp>
#include <Models/EmulatorWalletDataSource.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Models/SyncService.hpp>
#include <Models/WalletTransactionsListModel.hpp>
#include <Models/WalletMarketSwapModel.hpp>
#include <Networking/XSNBlockExplorerHttpClient.hpp>
#include <RPC/GRPCServer.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationPaymentNodesViewModel.hpp>
#include <ViewModels/EmulatorViewModel.hpp>
#include <ViewModels/LocalCurrencyViewModel.hpp>
#include <ViewModels/LockingViewModel.hpp>
#include <ViewModels/WalletViewModel.hpp>

//==============================================================================

static const bool DEFAULT_USE_EMULATOR = false;
static const bool USE_MOBILE = false;

static bool useStagingDev = false;

//==============================================================================

ApplicationViewModel::ApplicationViewModel(QObject* parent)
    : QObject(parent)
{
    _assetsModel.reset(new WalletAssetsModel(":/data/assets_conf.json"));
}

//==============================================================================

ApplicationViewModel::~ApplicationViewModel() {}

//==============================================================================

AbstractChainManager* ApplicationViewModel::chainManager() const
{
    return _ctx->chainManager();
}

//==============================================================================

WalletDataSource* ApplicationViewModel::dataSource() const
{
    return _ctx->walletDataSource();
}

//==============================================================================

AccountDataSource *ApplicationViewModel::accountDataSource() const
{
    return _ctx->accountDataSource();
}

//==============================================================================

AssetsTransactionsCache* ApplicationViewModel::transactionsCache() const
{
    return _ctx->transactionsCache();
}

//==============================================================================

WalletAssetsModel* ApplicationViewModel::assetsModel() const
{
    return _assetsModel.get();
}

//==============================================================================

AssetsBalance* ApplicationViewModel::assetsBalance() const
{
    return _ctx->assetsBalance();
}

//==============================================================================

LocalCurrency* ApplicationViewModel::localCurrency() const
{
    return _ctx->localCurrency();
}

//==============================================================================

ApplicationViewModel* ApplicationViewModel::Instance()
{
    static ApplicationViewModel instance;
    return &instance;
}

//==============================================================================

bool ApplicationViewModel::IsEmulated()
{
    return DEFAULT_USE_EMULATOR;
}

//==============================================================================

bool ApplicationViewModel::IsMobile()
{
    return USE_MOBILE;
}

//==============================================================================

void ApplicationViewModel::UseStagingEnv(bool staging)
{
    useStagingDev = staging;
}

//==============================================================================

bool ApplicationViewModel::IsStagingEnv()
{
    return useStagingDev;
}

//==============================================================================

QString ApplicationViewModel::clientID() const
{
    if (auto walletDataSource = dataSource()) {
        return walletDataSource->identityPubKey();
    }

    return QString();
}

//==============================================================================

WalletMarketSwapModel *ApplicationViewModel::marketSwapModel() const
{
    return _ctx->marketSwapModel();
}

//==============================================================================

LockingViewModel *ApplicationViewModel::lockingViewModel() const
{
    return _ctx->lockingViewModel();
}

//==============================================================================

TradingBotModel *ApplicationViewModel::tradingBotModel() const
{
    return _ctx->tradingBotModel();
}

//==============================================================================

void ApplicationViewModel::destroyContext()
{
    _ctx.reset();
    destroyContextFinished();
}

//==============================================================================

WalletViewModel* ApplicationViewModel::walletViewModel() const
{
    return _ctx->walletViewModel();
}

//==============================================================================

QObject* ApplicationViewModel::syncService() const
{
    return _ctx->syncService();
}

//==============================================================================

PaymentNodesManager *ApplicationViewModel::paymentNodesManager() const
{
    return _ctx->paymentNodesManager();
}

//==============================================================================

PaymentNodeStateManager* ApplicationViewModel::paymentNodeStateManager() const
{
    return _ctx->paymentNodeStateManager();
}

//==============================================================================

AssetsRemotePriceModel* ApplicationViewModel::currencyRates() const
{
    return _ctx->currencyRates();
}

//==============================================================================

bool ApplicationViewModel::isEmulated() const
{
    return IsEmulated();
}

//==============================================================================

LocalCurrencyViewModel* ApplicationViewModel::localCurrencyViewModel() const
{
    return _ctx->localCurrencyViewModel();
}

//==============================================================================

AbstractNetworkingFactory* ApplicationViewModel::apiClientsFactory() const
{
    return _ctx->apiClientsFactory();
}

//==============================================================================

QObject* ApplicationViewModel::emulatorViewModel() const
{
    return _ctx->emulatorViewModel();
}

//==============================================================================

DexService* ApplicationViewModel::dexService() const
{
    return _ctx->dexService();
}

//==============================================================================

ApplicationPaymentNodesViewModel* ApplicationViewModel::applicationPaymentNodeViewModel() const
{
    return _ctx->applicationPayNodeViewModel();
}

//==============================================================================

NotificationData* ApplicationViewModel::notificationData() const
{
    return _ctx->notificationData();
}

//==============================================================================

AbstractChainDataSource* ApplicationViewModel::chainDataSource() const
{
    return _ctx->chainDataSource();
}

//==============================================================================

DexStatusManager* ApplicationViewModel::dexStatusManager() const
{
    return _ctx->dexStatusManager();
}

//==============================================================================

ChannelRentalHelper* ApplicationViewModel::channelRentalHelper() const
{
    return _ctx->channelRentalHelper();
}

//==============================================================================

void ApplicationViewModel::loadContext()
{
    init();
    loadContextFinished();
}

//==============================================================================

void ApplicationViewModel::init()
{
    _ctx.reset(new Context(_assetsModel.get()));
}

//==============================================================================
