#ifndef EMULATEDCHAINSYNCMANAGER_HPP
#define EMULATEDCHAINSYNCMANAGER_HPP

#include <Chain/AbstractChainSyncManager.hpp>
#include <Chain/ChainSyncHelper.hpp>
#include <QObject>
#include <QQueue>
#include <Utils/Utils.hpp>

class WalletDataSource;
class ChainNotifications;
class EmulatorViewModel;
struct BlockFilterMatchable;
struct BlockIndex;

class EmulatorChainSyncManager : public AbstractChainSyncManager {
    Q_OBJECT
public:
    explicit EmulatorChainSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
        WalletDataSource& walletDataSource, const BlockFilterMatchable& blockFilterMatcher,
        CoinAsset asset, Utils::WorkerThread& workerThread,
        QPointer<AbstractBlockExplorerHttpClient> blockExplorerHttpClient,
        EmulatorViewModel& emulatorViewModel, QObject* parent = nullptr);

    void trySync() override;
    void interruptAsync() override;
    void interrupt() override;

signals:

public slots:

private slots:
    void onAPISyncError(QString error);
    void onHeadersSynced(std::vector<Wire::VerboseBlockHeader> headers);

private:
    void scheduleLastHeaders(QString bestBlockHash);

protected:
    const BlockFilterMatchable& _filterMatcher;
    std::map<BlockHash, boost::optional<Wire::StrippedBlock>> _pendingBlocks;
    WalletDataSource& _walletDataSource;

private:
    qobject_delete_later_unique_ptr<ChainSyncHelper> _chainSyncHelper;
    EmulatorViewModel& _emulatorViewModel;
};

#endif // EMULATEDCHAINSYNCMANAGER_HPP
