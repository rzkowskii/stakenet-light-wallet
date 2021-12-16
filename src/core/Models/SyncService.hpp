#ifndef SYNCSERVICE_HPP
#define SYNCSERVICE_HPP

#include <Data/CoinAsset.hpp>
#include <Utils/Utils.hpp>

#include <QDateTime>
#include <QObject>
#include <QPointer>
#include <QSet>
#include <memory>
#include <unordered_map>

class QTimer;
class AbstractChainSyncManagerFactory;
class AbstractChainSyncManager;
class WalletAssetsModel;
class AbstractMutableChainManager;
class SyncStateProvider;
class WalletDataSource;

namespace SyncServiceImpl {

//==============================================================================

class AbstractSyncProcessor : public QObject {
    Q_OBJECT
public:
    explicit AbstractSyncProcessor(CoinAsset coinAsset, AbstractChainSyncManager& chainSyncManager);
    ~AbstractSyncProcessor();
    virtual void startSyncTask() = 0;

    CoinAsset coinAsset() const;

signals:
    void taskStarted();
    void taskFinished();
    void taskFailed(QString error);
    void rescanUpdated(unsigned blocks);
    void bestBlockHeightChanged(unsigned bestBlockHeight);

protected:
    AbstractChainSyncManager& syncManager() const;

private:
    CoinAsset _coinAsset;
    AbstractChainSyncManager& _chainSyncManager;
};

//==============================================================================

class SyncProcessor : public AbstractSyncProcessor {
    Q_OBJECT
public:
    explicit SyncProcessor(CoinAsset coinAsset, AbstractChainSyncManager& chainSyncManager);
    ~SyncProcessor() override;
    void startSyncTask() override;

private slots:
    void onIsSyncingChanged();
};
}

//==============================================================================

class SyncService : public QObject {
    Q_OBJECT
public:
    explicit SyncService(AbstractChainSyncManagerFactory& syncManagerFactory,
        const AbstractMutableChainManager& chainManager, const WalletAssetsModel& assetModel,
        const WalletDataSource& dataSource, QObject* parent = nullptr);
    virtual ~SyncService();

    void start();
    void stop();
    void rescanChains(std::vector<AssetID> chains);

    bool isChainSynced(AssetID assetID) const;
    bool isActive() const;

public slots:
    SyncStateProvider* assetSyncProvider(AssetID assetID);
    void rescanChain(AssetID assetID);

signals:
    void syncTaskStarted(AssetID id);
    void syncTaskFinished(AssetID id);
    void syncTaskSkiped(AssetID id);
    void syncFailed(QString error);
    void syncStopped();

private slots:
    void onCurrentSyncProcessorTaskFinished(AssetID id);
    void scheduleSync();

private:
    AbstractChainSyncManager& chainSyncManager(AssetID assetID, bool rescan);
    void startInitialSync(CoinAsset coinAsset);
    void startSync(CoinAsset coinAsset);
    void rescanChain(CoinAsset coinAsset);

    using CompletionHandler = std::function<void(AssetID)>;
    using AbstractSyncProcessorPtr
        = std::unique_ptr<SyncServiceImpl::AbstractSyncProcessor>;
    void runSyncProcessor(
        AbstractSyncProcessorPtr processor, CompletionHandler onCompletion = CompletionHandler());

private:
    using AbstractChainSyncManagerPtr = qobject_delete_later_unique_ptr<AbstractChainSyncManager>;
    using SyncStateProviderPtr = qobject_delete_later_unique_ptr<SyncStateProvider>;
    using IsRescan = bool;
    using ChainSyncManagerEntry = std::pair<AbstractChainSyncManagerPtr, IsRescan>;

    QTimer* _syncTimer = nullptr;
    const WalletDataSource& _dataSource;
    std::map<AssetID, std::pair<QString, QDateTime>> _syncState;
    AbstractChainSyncManagerFactory& _syncManagerFactory;
    std::unordered_map<AssetID, ChainSyncManagerEntry> _syncManagerMap;
    std::unordered_map<AssetID, AbstractSyncProcessorPtr> _currentSyncProcessor;
    const WalletAssetsModel& _assetModel;
    const AbstractMutableChainManager& _chainManager;
    std::unordered_map<AssetID, SyncStateProviderPtr> _assetSyncProviders;
};

//==============================================================================

#endif // SYNCSERVICE_HPP
