#ifndef ABSTRACTCHAINSYNCMANAGER_HPP
#define ABSTRACTCHAINSYNCMANAGER_HPP

#include <Data/CoinAsset.hpp>
#include <QObject>

class Chain;
class AbstractTransactionsCache;
class AssetsTransactionsCache;

namespace Utils {
class WorkerThread;
}

class AbstractChainSyncManager : public QObject {
    Q_OBJECT
public:
    explicit AbstractChainSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
        CoinAsset asset, QObject* parent = nullptr);
    virtual ~AbstractChainSyncManager();

    virtual void trySync() = 0;
    virtual void interruptAsync() = 0;
    virtual void interrupt() = 0;
    bool isSyncing() const;

signals:
    void syncProgress(unsigned current);
    void isSyncingChanged();
    void finished();
    void syncError(QString error);
    void bestBlockHeightChanged(unsigned bestBlockHeight);

protected:
    Chain& chain() const;
    AbstractTransactionsCache& txCache() const;
    AssetsTransactionsCache& assetsTxCache() const;
    const CoinAsset& coinAsset() const;
    void setIsSyncing(bool syncing);

private:
    Chain& _chain;
    AssetsTransactionsCache& _transactionsCache;
    CoinAsset _asset;
    bool _isSyncing{ false };
};

#endif // ABSTRACTCHAINSYNCMANAGER_HPP
