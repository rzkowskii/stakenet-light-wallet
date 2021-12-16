#ifndef ASSETTRANSACTIONSDATASOURCE_HPP
#define ASSETTRANSACTIONSDATASOURCE_HPP

#include <Data/TransactionEntry.hpp>
#include <Tools/Common.hpp>

#include <QObject>

class AbstractTransactionsCache;

class AssetTransactionsDataSource : public QObject {
    Q_OBJECT
public:
    explicit AssetTransactionsDataSource(
        AssetID assetID, QPointer<AbstractTransactionsCache> dataSource, QObject* parent = nullptr);
    void fetchTransactions();

    const TransactionsList& transactionsList() const;
    AssetID assetID() const;

signals:
    void transactionsFetched();
    void txnsAdded(const std::vector<Transaction>& tx);
    void txnsChanged(const std::vector<Transaction>& tx, std::vector<int> indexes);

private slots:
    void onTransactionAdded(std::vector<Transaction> txns);
    void onTransactionChanged(std::vector<Transaction> txns);

private:
    QPointer<AbstractTransactionsCache> _dataSource;
    AssetID _assetID;
    TransactionsList _transactions;
};

#endif // ASSETTRANSACTIONSDATASOURCE_HPP
