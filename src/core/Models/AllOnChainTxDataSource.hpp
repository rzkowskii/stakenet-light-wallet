#ifndef ALLTRANSACTIONSDATASOURCE_HPP
#define ALLTRANSACTIONSDATASOURCE_HPP

#include <Data/TransactionEntry.hpp>
#include <Utils/Utils.hpp>

#include <QObject>

class AssetsTransactionsCache;
class AbstractTransactionsCache;

class AllOnChainTxDataSource : public QObject {
    Q_OBJECT
public:
    explicit AllOnChainTxDataSource(
        QPointer<AssetsTransactionsCache> dataSource, QObject* parent = nullptr);
    virtual void fetchTransactions();
    const OnChainTxMap& allTransactions() const;

signals:
    void transactionsFetched();
    void txnsAdded(const std::vector<OnChainTxRef>& transaction);
    void txnsChanged(const std::vector<OnChainTxRef>& transaction);

public slots:

private slots:
    void onTxnsAdded(AssetID assetID, const std::vector<OnChainTxRef>& txns);
    void onTxnsChanged(AssetID assetID, const std::vector<OnChainTxRef>& txns);
    void onNewCacheAdded(AssetID assetID);

private:
    Promise<void> connectAndUpdateNewCache(AssetID assetID, AbstractTransactionsCache* cache);

private:
    QPointer<AssetsTransactionsCache> _dataSource;
    OnChainTxMap _transactions;
};

#endif // ALLTRANSACTIONSDATASOURCE_HPP
