#include "AssetTransactionsDataSource.hpp"
#include <Chain/AbstractTransactionsCache.hpp>

//==============================================================================

template <class Container> static int IndexOf(const Container& where, const Transaction& tx)
{
    auto it = std::find(std::begin(where), std::end(where), tx);

    return it == std::end(where) ? -1 : std::distance(std::begin(where), it);
}

//==============================================================================

AssetTransactionsDataSource::AssetTransactionsDataSource(
    AssetID assetID, QPointer<AbstractTransactionsCache> dataSource, QObject* parent)
    : QObject(parent)
    , _dataSource(dataSource)
    , _assetID(assetID)
{
    connect(dataSource, &AbstractTransactionsCache::txnsAdded, this,
        &AssetTransactionsDataSource::onTransactionAdded);
    connect(dataSource, &AbstractTransactionsCache::txnsChanged, this,
        &AssetTransactionsDataSource::onTransactionChanged);
}

//==============================================================================

void AssetTransactionsDataSource::fetchTransactions()
{
    _dataSource->transactionsList().then([this](TransactionsList transactions) {
        _transactions = transactions;
        transactionsFetched();
    });
}

//==============================================================================

const TransactionsList& AssetTransactionsDataSource::transactionsList() const
{
    return _transactions;
}

//==============================================================================

AssetID AssetTransactionsDataSource::assetID() const
{
    return _assetID;
}

//==============================================================================

void AssetTransactionsDataSource::onTransactionChanged(std::vector<Transaction> txns)
{
    std::vector<Transaction> filtered;
    std::vector<int> indexes;
    for (auto&& tx : txns) {
        auto index = IndexOf(_transactions, tx);
        if (index >= 0) {
            _transactions[index].swap(tx);
            filtered.emplace_back(tx);
            indexes.emplace_back(index);
        }
    }

    txnsChanged(filtered, indexes);
}

//==============================================================================

void AssetTransactionsDataSource::onTransactionAdded(std::vector<Transaction> txns)
{
    _transactions.reserve(_transactions.size() + txns.size());
    std::copy(std::begin(txns), std::end(txns), std::back_inserter(_transactions));
    txnsAdded(txns);
}

//==============================================================================
