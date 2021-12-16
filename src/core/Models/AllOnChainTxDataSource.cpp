#include "AllOnChainTxDataSource.hpp"
#include <Chain/AbstractTransactionsCache.hpp>

//==============================================================================

AllOnChainTxDataSource::AllOnChainTxDataSource(
    QPointer<AssetsTransactionsCache> dataSource, QObject* parent)
    : QObject(parent)
    , _dataSource(dataSource)
{
}

//==============================================================================

void AllOnChainTxDataSource::fetchTransactions()
{
    std::vector<Promise<void>> promises;
    for (auto&& assetID : _dataSource->availableCaches()) {
        promises.emplace_back(
            _dataSource->cacheById(assetID).then([this, assetID](AbstractTransactionsCache* cache) {
                return this->connectAndUpdateNewCache(assetID, cache);
            }));
    }

    connect(_dataSource, &AssetsTransactionsCache::cacheAdded, this,
        &AllOnChainTxDataSource::onNewCacheAdded, Qt::UniqueConnection);

    QtPromise::all(promises).then([this] { this->transactionsFetched(); });
}

//==============================================================================

const OnChainTxMap& AllOnChainTxDataSource::allTransactions() const
{
    return _transactions;
}

//==============================================================================

void AllOnChainTxDataSource::onTxnsAdded(AssetID assetID, const std::vector<OnChainTxRef>& txns)
{
    auto& where = _transactions[assetID];
    std::copy(std::begin(txns), std::end(txns), std::back_inserter(where));
    txnsAdded(txns);
}

//==============================================================================

void AllOnChainTxDataSource::onTxnsChanged(AssetID assetID, const std::vector<OnChainTxRef>& txns)
{
    auto& list = _transactions[assetID];
    for (const auto& transaction : txns) {
        auto txid = transaction->txId();
        std::replace_if(std::begin(list), std::end(list),
            [txid](const auto& ref) { return txid == ref->txId(); }, transaction);
    }
    txnsChanged(txns);
}

//==============================================================================

void AllOnChainTxDataSource::onNewCacheAdded(AssetID assetID)
{
    _dataSource->cacheById(assetID).then([=](AbstractTransactionsCache* cache) {
        this->connectAndUpdateNewCache(assetID, cache).then([this, assetID] {
            if (!_transactions.at(assetID).empty()) {
                this->transactionsFetched();
            }
        });
    });
}

//==============================================================================

Promise<void> AllOnChainTxDataSource::connectAndUpdateNewCache(
    AssetID assetID, AbstractTransactionsCache* cache)
{
    connect(cache, &AbstractTransactionsCache::onChainTxnsAdded, this,
        std::bind(&AllOnChainTxDataSource::onTxnsAdded, this, assetID, std::placeholders::_1));
    connect(cache, &AbstractTransactionsCache::onChainTxnsChanged, this,
        std::bind(&AllOnChainTxDataSource::onTxnsChanged, this, assetID, std::placeholders::_1));

    return cache->onChainTransactionsList().then(
        [this, assetID](OnChainTxList transactions) { _transactions[assetID].swap(transactions); });
}

//==============================================================================
