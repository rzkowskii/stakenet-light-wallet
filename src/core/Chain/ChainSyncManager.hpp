#ifndef CHAINSYNCMANAGER_HPP
#define CHAINSYNCMANAGER_HPP

#include <QObject>
#include <QPointer>
#include <QQueue>
#include <memory>
#include <queue>
#include <stack>

#include <Chain/AbstractChainDataSource.hpp>
#include <Chain/AbstractChainSyncManager.hpp>
#include <Chain/BlockFilterMatcher.hpp>
#include <Chain/BlockHeader.hpp>
#include <Chain/ChainSyncHelper.hpp>
#include <Utils/Utils.hpp>

class WalletDataSource;
class AbstractChainDataSource;
class ChainNotifications;
struct BlockFilterMatchable;
struct BlockIndex;

class ChainSyncManager : public AbstractChainSyncManager {
    Q_OBJECT
public:
    explicit ChainSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
        WalletDataSource& walletDataSource, const BlockFilterMatchable& blockFilterMatcher,
        CoinAsset asset, AbstractChainDataSource& chainDataSource, QObject* parent = nullptr);
    ~ChainSyncManager() override;

public:
    void trySync() override;
    void interruptAsync() override;
    void interrupt() override;

protected:
    void scheduleStrippedBlock(BlockHash blockHash, int64_t blockHeight);
    void scheduleLastHeaders(QString bestBlockHash);
    const BlockFilterMatcher& filterMatcher();

    void setBestBlockHeight(size_t bestBlockHeight);
    size_t bestBlockHeight() const;

private slots:
    void onHeadersSynced(std::vector<Wire::VerboseBlockHeader> headers);
    void onAPISyncError(QString error);
    void onStrippedBlockSynced(Wire::StrippedBlock block);

private:
    void syncBestBlockHash();
    void processHeaders();
    void reorg(BlockHash reorgHash);
    using ForkBlock = std::tuple<BlockHash, size_t, std::vector<Wire::VerboseBlockHeader>>;
    Promise<ForkBlock> findForkBlock(BlockHash localBestBlock, size_t reorgHeight,
        std::vector<Wire::VerboseBlockHeader> cachedHeaders) const;

protected:
    const BlockFilterMatchable& _filterMatcher;
    std::map<BlockHash, boost::optional<Wire::StrippedBlock>> _pendingBlocks;
    BFMatcherUniqueRef _cachedMatcher;
    WalletDataSource& _walletDataSource;
    AbstractChainDataSource& _chainDataSource;
    size_t _bestBlockHeight{ 0 };

private:
    std::queue<Wire::VerboseBlockHeader> _headersProcessingQueue;
    //    WalletDataSource &_walletDataSource;
    //    BFMatcherUniqueRef _cachedMatcher;
};

class RescanSyncManager : public ChainSyncManager {
    Q_OBJECT
public:
    explicit RescanSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
        WalletDataSource& walletDataSource, const BlockFilterMatchable& blockFilterMatcher,
        CoinAsset asset, AbstractChainDataSource& chainDataSource, QObject* parent = nullptr);

    void trySync() override;
    void interruptAsync() override;
};

#endif // CHAINSYNCMANAGER_HPP
