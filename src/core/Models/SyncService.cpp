#include "SyncService.hpp"
#include <Chain/AbstractChainManager.hpp>
#include <Chain/AbstractChainSyncManager.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/ChainSyncManagerFactory.hpp>
#include <Models/SyncStateProvider.hpp>
#include <Models/WalletDataSource.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>

#include <QDateTime>
#include <QTimer>

//==============================================================================

namespace SyncState {
static const QString INITIAL("INITIAL");
static const QString SYNCED("SYNCED");
static const QString RESCAN("RESCAN");
}

//==============================================================================

SyncService::SyncService(AbstractChainSyncManagerFactory& syncManagerFactory,
    const AbstractMutableChainManager& chainManager, const WalletAssetsModel& assetModel,
    const WalletDataSource& dataSource, QObject* parent)
    : QObject(parent)
    , _dataSource(dataSource)
    , _syncManagerFactory(syncManagerFactory)
    , _assetModel(assetModel)
    , _chainManager(chainManager)
{
    _syncTimer = new QTimer(this);
    connect(_syncTimer, &QTimer::timeout, this, &SyncService::scheduleSync);
}

//==============================================================================

SyncService::~SyncService() {}

//==============================================================================

void SyncService::start()
{
    LogCDebug(Sync) << "Starting SyncService";
    _syncTimer->start(2000);
}

//==============================================================================

void SyncService::stop()
{
    _syncTimer->stop();
    _currentSyncProcessor.clear();
    syncStopped();
}

//==============================================================================

bool SyncService::isActive() const
{
    return _syncTimer->isActive();
}

//==============================================================================

SyncStateProvider* SyncService::assetSyncProvider(AssetID assetID)
{
    if (_assetSyncProviders.count(assetID) == 0) {
        _assetSyncProviders.emplace(assetID, SyncStateProviderPtr(new SyncStateProvider(this)));
    }

    return _assetSyncProviders.at(assetID).get();
}

//==============================================================================

void SyncService::rescanChain(AssetID assetID)
{
    rescanChains({ assetID });
}

//==============================================================================

void SyncService::onCurrentSyncProcessorTaskFinished(AssetID id)
{
    LogCDebug(Sync) << "Current sync task finished" << id;
    syncTaskFinished(id);
}

//==============================================================================

void SyncService::scheduleSync()
{
    if (_chainManager.chains().empty()) {
        LogCDebug(Sync) << "Skipping sync, chainManager not loaded";
        return;
    }

    for (auto&& id : _assetModel.activeAssets()) {
        const auto& asset = _assetModel.assetById(id);
        if (!_chainManager.hasChain(id)) {
            LogCDebug(Sync) << "Skipping asset" << id << "no chain found";
            continue;
        }

        if (asset.token()) {
            // don't sync tokens, expect parent asset to sync it
            continue;
        }

        if (_currentSyncProcessor.count(asset.coinID()) > 0) {
            continue;
        }

        if (_syncState.count(id) > 0) {
            auto state = _syncState.at(id).first;
            if (state == SyncState::RESCAN) {
                if (_dataSource.getAddressTypeSync(asset.coinID()) == Enums::AddressType::NONE) {
                    continue;
                }

                rescanChain(asset);
            } else if (state == SyncState::SYNCED) {
                if (_syncState.at(id).second.secsTo(QDateTime::currentDateTimeUtc()) > 20) {
                    startSync(asset);
                }
            }
        } else {
            _syncState.emplace(id, std::make_pair(SyncState::INITIAL, QDateTime()));
            startSync(asset);
        }
    }
}

//==============================================================================

void SyncService::startSync(CoinAsset coinAsset)
{
    LogCDebug(Sync) << "Starting sync for coin" << coinAsset.name();
    auto onCompletion = [this](AssetID id) {
        if (_syncState[id].first == SyncState::RESCAN) {
            return;
        }

        _syncState[id] = { SyncState::SYNCED, QDateTime::currentDateTimeUtc() };
    };

    AbstractSyncProcessorPtr syncProcessor(
        new SyncServiceImpl::SyncProcessor(coinAsset, chainSyncManager(coinAsset.coinID(), false)));

    auto syncStateProvider = assetSyncProvider(coinAsset.coinID());

    connect(syncProcessor.get(), &SyncServiceImpl::SyncProcessor::taskStarted, this,
        std::bind(&SyncStateProvider::setSyncing, syncStateProvider, true));
    connect(syncProcessor.get(), &SyncServiceImpl::SyncProcessor::taskFinished, this,
        std::bind(&SyncStateProvider::setSyncing, syncStateProvider, false));
    connect(syncProcessor.get(), &SyncServiceImpl::SyncProcessor::taskFailed, this,
        &SyncService::syncFailed);

    runSyncProcessor(std::move(syncProcessor), onCompletion);
}

//==============================================================================

void SyncService::rescanChain(CoinAsset coinAsset)
{
    LogCDebug(Sync) << "Starting rescan for coin" << coinAsset.name();
    auto onCompletion = [this](AssetID id) {
        _syncState[id] = { SyncState::SYNCED, QDateTime::currentDateTimeUtc() };
    };

    AbstractSyncProcessorPtr syncProcessor(
        new SyncServiceImpl::SyncProcessor(coinAsset, chainSyncManager(coinAsset.coinID(), true)));

    auto syncStateProvider = assetSyncProvider(coinAsset.coinID());

    connect(syncProcessor.get(), &SyncServiceImpl::SyncProcessor::taskStarted, this,
        std::bind(&SyncStateProvider::setScanning, syncStateProvider, true));
    connect(syncProcessor.get(), &SyncServiceImpl::SyncProcessor::taskFinished, this,
        std::bind(&SyncStateProvider::setScanning, syncStateProvider, false));
    connect(syncProcessor.get(), &SyncServiceImpl::SyncProcessor::taskFailed, this,
        &SyncService::syncFailed);
    connect(syncProcessor.get(), &SyncServiceImpl::SyncProcessor::rescanUpdated, syncStateProvider,
        &SyncStateProvider::setRescanProgress);
    connect(syncProcessor.get(), &SyncServiceImpl::SyncProcessor::bestBlockHeightChanged,
        syncStateProvider, &SyncStateProvider::setBestBlockHeight);

    runSyncProcessor(std::move(syncProcessor), onCompletion);
}

//==============================================================================

void SyncService::runSyncProcessor(
    AbstractSyncProcessorPtr processor, CompletionHandler onCompletion)
{
    auto processorPtr = processor.get();
    auto assetID = processor->coinAsset().coinID();
    LogCDebug(Sync) << "Asked to run new sync processor" << processorPtr;
    _currentSyncProcessor.emplace(assetID, std::move(processor));

    // this can be called as interrupt handler when destroying object, we need to make sure that
    // it's not called from destructor. If it's a direct connection we will get double free.
    connect(processorPtr, &SyncServiceImpl::AbstractSyncProcessor::taskFinished, this,
        [this, assetID, onCompletion] {
            if (_currentSyncProcessor.count(assetID) == 0) {
                return;
            }

            auto processor = _currentSyncProcessor.at(assetID).get();
            LogCDebug(Sync) << "Sync processor task finished" << processor;
            _currentSyncProcessor.erase(assetID);

            if (onCompletion)
                onCompletion(assetID);

            onCurrentSyncProcessorTaskFinished(assetID);
        },
        Qt::QueuedConnection);

    connect(processorPtr, &SyncServiceImpl::AbstractSyncProcessor::taskStarted, this,
        std::bind(&SyncService::syncTaskStarted, this, assetID));

    LogCDebug(Sync) << "Starting sync task";
    processorPtr->startSyncTask();
}

//==============================================================================

AbstractChainSyncManager& SyncService::chainSyncManager(AssetID assetID, bool rescan)
{
    if (_syncManagerMap.find(assetID) == _syncManagerMap.end()) {
        auto& chain = _chainManager.chainById(assetID);
        _syncManagerMap.emplace(assetID,
            std::make_pair(rescan ? _syncManagerFactory.createRescanSyncManager(chain)
                                  : _syncManagerFactory.createAPISyncManager(chain),
                rescan));
    }

    // Store only one sync manager, rescan or api, if it's different then delete old and recreate.

    const auto& syncManager = _syncManagerMap.at(assetID);
    if (syncManager.second != rescan) {
        _syncManagerMap.erase(assetID);
        return chainSyncManager(assetID, rescan);
    }

    return *syncManager.first;
}

//==============================================================================

void SyncService::rescanChains(std::vector<AssetID> chains)
{
    for (auto&& chain : chains) {
        if (_assetModel.hasAsset(chain)) {
            _syncState[chain] = std::make_pair(SyncState::RESCAN, QDateTime());
        }
    }
}

//==============================================================================

bool SyncService::isChainSynced(AssetID assetID) const
{
    if (_syncState.count(assetID) > 0) {
        return _syncState.at(assetID).first == SyncState::SYNCED;
    } else if (auto token = _assetModel.assetById(assetID).token()) {
        return isChainSynced(token->chainID());
    }

    return false;
}

//==============================================================================

SyncServiceImpl::AbstractSyncProcessor::AbstractSyncProcessor(
    CoinAsset coinAsset, AbstractChainSyncManager& chainSyncManager)
    : QObject(nullptr)
    , _coinAsset(coinAsset)
    , _chainSyncManager(chainSyncManager)
{
}

//==============================================================================

SyncServiceImpl::AbstractSyncProcessor::~AbstractSyncProcessor() {}

//==============================================================================

CoinAsset SyncServiceImpl::AbstractSyncProcessor::coinAsset() const
{
    return _coinAsset;
}

//==============================================================================

AbstractChainSyncManager& SyncServiceImpl::AbstractSyncProcessor::syncManager() const
{
    return _chainSyncManager;
}

//==============================================================================

SyncServiceImpl::SyncProcessor::SyncProcessor(
    CoinAsset coinAsset, AbstractChainSyncManager& chainSyncManager)
    : AbstractSyncProcessor(coinAsset, chainSyncManager)
{
}

//==============================================================================

SyncServiceImpl::SyncProcessor::~SyncProcessor()
{
    syncManager().interrupt();
}

//==============================================================================

void SyncServiceImpl::SyncProcessor::startSyncTask()
{
    connect(&syncManager(), &AbstractChainSyncManager::isSyncingChanged, this,
        &SyncProcessor::onIsSyncingChanged, Qt::UniqueConnection);
    connect(&syncManager(), &AbstractChainSyncManager::syncError, this, &SyncProcessor::taskFailed,
        Qt::UniqueConnection);
    connect(&syncManager(), &AbstractChainSyncManager::syncProgress, this,
        &SyncProcessor::rescanUpdated, Qt::UniqueConnection);
    connect(&syncManager(), &AbstractChainSyncManager::bestBlockHeightChanged, this,
        &SyncProcessor::bestBlockHeightChanged, Qt::UniqueConnection);

    syncManager().trySync();
}

//==============================================================================

void SyncServiceImpl::SyncProcessor::onIsSyncingChanged()
{
    if (syncManager().isSyncing()) {
        taskStarted();
    } else {
        taskFinished();
    }
}

//==============================================================================
