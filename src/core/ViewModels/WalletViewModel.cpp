#include <Chain/AbstractAssetAccount.hpp>
#include <Chain/AbstractChainManager.hpp>
#include <Chain/Chain.hpp>
#include <Chain/TransactionsCache.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Models/SyncService.hpp>
#include <Models/WalletDataSource.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>
#include <ViewModels/WalletViewModel.hpp>

#include <QDateTime>
#include <QFileInfo>
#include <QProcess>

//==============================================================================

static void ResetChain(bool isEmulated)
{
    auto path = GetChainIndexDir(isEmulated);
    if (!path.isEmpty()) {
        QDir dir(path);
        dir.removeRecursively();
    }
}

//==============================================================================

static void ResetTransactions(bool isEmulated)
{
    auto path = GetTxDBPath(isEmulated);
    if (!path.isEmpty()) {
        QDir dir(path);
        dir.removeRecursively();
    }
}

//==============================================================================

WalletViewModel::WalletViewModel(WalletDataSource& dataSource, UTXOSetDataSource& utxoDataSource,
    AssetsTransactionsCache& transactionsCache, AssetsAccountsCache& accountsCache,
    WalletAssetsModel& walletAssetsModel, AbstractMutableChainManager& chainManager,
    SyncService& syncService, QObject* parent)
    : QObject(parent)
    , _walletDataSource(dataSource)
    , _utxoDataSource(utxoDataSource)
    , _transactionsCache(transactionsCache)
    , _accountsCache(accountsCache)
    , _walletAssetsModel(walletAssetsModel)
    , _chainManager(chainManager)
    , _syncService(syncService)
{
    for (auto&& asset : _walletAssetsModel.assets()) {
        auto assetID = asset.coinID();
        auto settings = _walletAssetsModel.assetSettings(assetID);
        connect(settings, &AssetSettings::activeChanged, this, [this, assetID](bool active) {
            if (active && !_chainManager.hasChain(assetID)) {
                _chainManager.loadChains({ assetID });
            }
        });
    }
}

//==============================================================================

WalletViewModel::~WalletViewModel() {}

//==============================================================================

QString WalletViewModel::walletAge()
{
    QDir dbDir(QFileInfo(QString::fromStdString(GetPathForWalletDb(false))).absoluteDir());
    dbDir.cd("wallets");
    auto walletLock = QFileInfo(dbDir, "wallet.dat");
    return DateDifference(walletLock.birthTime(), QDateTime::currentDateTime());
}

//==============================================================================

void WalletViewModel::createWalletWithMnemonic()
{
    _walletDataSource.createWalletWithMnemonic().then([this] {
        loadChains().then([this] {
            loadTransactionsDatabase().then([this] {
                loadAccountsDatabase().then([this] {
                    createdChanged(true);
                    LogCDebug(General) << "Wallet created!";
                    startSync();
                    emit walletLoaded();
                });
            });
        });
    });
}

//==============================================================================

void WalletViewModel::requestMnemonic()
{
    _walletDataSource.getMnemonic()
        .then([this](QString mnemonic) { requestMnemonicFinished(mnemonic); })
        .fail([this](QString) { requestMnemonicFinished(QString()); });
}

//==============================================================================

bool WalletViewModel::verifyMnemonic(QString mnemonic)
{
    bool result = false;
    _walletDataSource.getMnemonic()
        .then([&result, &mnemonic](QString gotMnemonic) { result = (mnemonic == gotMnemonic); })
        .wait();

    return result;
}

//==============================================================================

void WalletViewModel::restoreWallet(QString mnemonic)
{
    _walletDataSource.restoreWalletWithMnemAndPass(mnemonic)
        .then([this] {
            loadChains().then([this] {
                loadTransactionsDatabase().then([this] {
                    loadAccountsDatabase().then([this] {
                        restoredChanged(true);
                        _syncService.rescanChains(_chainManager.chains());
                        startSync();
                        walletLoaded();
                        return Promise<void>([](const auto& resolver, const auto&) { resolver(); });
                    });
                });
            });
        })
        .fail([this](const std::exception& ex) {
            LogCDebug(WalletBackend) << "Failed to restore wallet" << ex.what();
            restoredChanged(false);
        });
}

//==============================================================================

void WalletViewModel::encryptWallet(QString password)
{
    _walletDataSource.encryptWallet(SecureString(password.toStdString()))
        .then([this] { emit walletEncrypted(); })
        .fail([this](const std::exception& ex) {
            LogCDebug(WalletBackend) << "Failed to encrypt wallet" << ex.what();
            encryptWalletFailed(QString::fromStdString(ex.what()));
        });
}

//==============================================================================

void WalletViewModel::loadApp(QString password)
{
    if (_walletDataSource.isCreated()) {
        loadWallet(static_cast<SecureString>(password.toStdString()))
            .then([this] {
                loadChains().then([this] {
                    loadTransactionsDatabase().then([this] {
                        return _utxoDataSource.load().then([this] {
                            loadAccountsDatabase().then([this] {
                                return filteredChains(_chainManager.chains())
                                    .then([this](std::vector<AssetID> filteredChains) {
                                        _syncService.rescanChains(filteredChains);
                                        loadedChanged(true);
                                        startSync();
                                    });
                            });
                        });
                    });
                });
            })
            .fail([this](std::exception& ex) {
                this->loadingWalletFailed(QString::fromStdString(ex.what()));
            });
    } else {
        loadedChanged(false);
    }
}

//==============================================================================

void WalletViewModel::resetState(WalletViewModel::LoadingState loadingState)
{
    QTimer::singleShot(0, [loadingState] {
        ApplicationViewModel::Instance()->destroyContext();

        auto isEmulated = ApplicationViewModel::IsEmulated();
        if (loadingState == LoadingState::Chain || loadingState == LoadingState::Transactions) {
            QString path;
            if (loadingState == LoadingState::Chain) {
                ResetChain(isEmulated);
            } else if (loadingState == LoadingState::Transactions) {
                ResetTransactions(isEmulated);
            } else {
                LogCDebug(General) << "Unknown loading state" << loadingState;
                return;
            }
        } else if (loadingState == LoadingState::Wallet) {
            ResetWallet(isEmulated);
        } else if (loadingState == LoadingState::Done) {
            ResetChain(isEmulated);
            ResetTransactions(isEmulated);
            ResetWallet(isEmulated);
        }

        ApplicationViewModel::Instance()->loadContext();
    });
}

//==============================================================================

void WalletViewModel::startSync()
{
    if (!_syncService.isActive() && _walletDataSource.isLoaded()) {
        if (!ApplicationViewModel::IsEmulated()) {
            _syncService.start();
        }
    }
}

//==============================================================================

Promise<void> WalletViewModel::loadChains()
{
    std::vector<AssetID> chains = _walletAssetsModel.activeAssets();

    //    bool canBoostrap = false;
    //    QString path;
    //    std::tie(canBoostrap, path) = canBootstrap();

    auto loadChainsHelper = [chains, this] {
        loadingProcessChanged(LoadingState::Chain);
        return _chainManager.loadChains(chains).tapFail(
            [this] { loadingProcessFailed(LoadingState::Chain); });
    };

#if 0
    if(canBoostrap)
    {
        loadingProcessChanged("bootstrap");
        return _chainManager.loadFromBootstrap(path).then([loadChainsHelper, this] {
            _syncService.rescanChains(_chainManager.chains());
            startSync();
            emit walletLoaded();
            return loadChainsHelper();
        }).tapFail([this]{
            loadingProcessFailed("bootstrap");
        });
    }

#endif

    return loadChainsHelper();
}

//==============================================================================

Promise<void> WalletViewModel::loadTransactionsDatabase()
{
    loadingProcessChanged(LoadingState::Transactions);
    return _transactionsCache.load(_walletDataSource.isCreated() == false)
        .tap([this] { this->transactionsDatabaseLoaded(); })
        .tapFail([this] { loadingProcessFailed(LoadingState::Transactions); });
}

//==============================================================================

Promise<void> WalletViewModel::loadAccountsDatabase()
{
    return _accountsCache.load();
}

//==============================================================================

Promise<void> WalletViewModel::loadWallet(SecureString password)
{
    loadingProcessChanged(LoadingState::Wallet);
    return _walletDataSource.loadWallet(password)
        .tap([this]() { emit walletLoaded(); })
        .tapFail([this] { loadingProcessFailed(LoadingState::Wallet); });
}

//==============================================================================

Promise<std::vector<AssetID>> WalletViewModel::filteredChains(std::vector<AssetID> chains)
{
    return QtPromise::filter(chains, [this](AssetID assetID, int /*index*/) {
        if (_walletDataSource.isEmpty(assetID)) {
            return QtPromise::resolve(false);
        }
        return _chainManager
            .getChainView(assetID, AbstractChainManager::ChainViewUpdatePolicy::CompressedEvents)
            .then([](std::shared_ptr<ChainView> chainView) {
                return chainView->bestBlockHash().then(
                    [](QString bbhash) { return bbhash.isEmpty(); });
            });
    });
}

//==============================================================================

std::tuple<bool, QString> WalletViewModel::canBootstrap() const
{
    auto dir = QDir::current();
    const QString fileName("bootstrap.dat");
    LogCDebug(General) << "canBootStrap" << QDir::current() << dir.absoluteFilePath(fileName);
    return std::make_tuple(dir.exists(fileName), dir.absoluteFilePath(fileName));
}

//==============================================================================
