#include "AbstractChainSyncManager.hpp"

#include <Chain/AbstractTransactionsCache.hpp>
#include <Utils/Utils.hpp>

//==============================================================================

AbstractChainSyncManager::AbstractChainSyncManager(
    Chain& chain, AssetsTransactionsCache& transactionsCache, CoinAsset asset, QObject* parent)
    : QObject(parent)
    , _chain(chain)
    , _transactionsCache(transactionsCache)
    , _asset(asset)
{
}

//==============================================================================

AbstractChainSyncManager::~AbstractChainSyncManager() {}

//==============================================================================

bool AbstractChainSyncManager::isSyncing() const
{
    return _isSyncing;
}

//==============================================================================

Chain& AbstractChainSyncManager::chain() const
{
    return _chain;
}

//==============================================================================

AbstractTransactionsCache& AbstractChainSyncManager::txCache() const
{
    return _transactionsCache.cacheByIdSync(coinAsset().coinID());
}

//==============================================================================

AssetsTransactionsCache &AbstractChainSyncManager::assetsTxCache() const
{
     return _transactionsCache;
}

//==============================================================================

const CoinAsset& AbstractChainSyncManager::coinAsset() const
{
    return _asset;
}

//==============================================================================

void AbstractChainSyncManager::setIsSyncing(bool syncing)
{
    if (_isSyncing != syncing) {
        _isSyncing = syncing;
        isSyncingChanged();
        if (!isSyncing()) {
            finished();
        }
    }
}

//==============================================================================
