#ifndef ABSTRACTTRANSACTIONSCACHE_HPP
#define ABSTRACTTRANSACTIONSCACHE_HPP

#include <Data/TransactionEntry.hpp>
#include <QObject>
#include <Utils/Utils.hpp>
#include <boost/optional.hpp>

namespace Utils {
class WorkerThread;
}

//==============================================================================

class AbstractTransactionsCache : public QObject {
    Q_OBJECT
public:
    explicit AbstractTransactionsCache(QObject* parent = nullptr);

    /** all transactions **/
    virtual Promise<TransactionsList> transactionsList() const = 0;
    virtual Promise<void> addTransactions(std::vector<Transaction> transaction) = 0;

    /** all transactions, sync interface **/
    virtual void addTransactionsSync(std::vector<Transaction> transaction) = 0;

    /** on chain transactions **/
    virtual Promise<OnChainTxList> onChainTransactionsList() const = 0;
    virtual Promise<OnChainTxRef> transactionById(QString txId) const = 0;
    virtual Promise<std::vector<QString>> transactionsInBlock(BlockHash blockHash) const = 0;

    /** on chain transactions, sync interface **/
    virtual OnChainTxRef transactionByIdSync(QString txId) const = 0;
    virtual const OnChainTxList& onChainTransactionsListSync() const = 0;
    virtual std::vector<QString> transactionsInBlockSync(BlockHash blockHash) const = 0;

    /** ln off chain transactions **/
    virtual Promise<LightningPaymentList> lnPaymentsList() const = 0;
    virtual Promise<LightningInvoiceList> lnInvoicesList() const = 0;

    /** ln off chain transactions, sync interface **/
    virtual const LightningPaymentList& lnPaymentsListSync() const = 0;
    virtual const LightningInvoiceList& lnInvoicesListSync() const = 0;

    /** eth on chain transactions **/
    virtual Promise<EthOnChainTxList> onEthChainTransactionsList() const = 0;
    virtual Promise<EthOnChainTxRef> ethTransactionById(QString txId) const = 0;

    /** eth on chain transactions, sync interface **/
    virtual EthOnChainTxRef ethTransactionByIdSync(QString txId) const = 0;
    virtual const EthOnChainTxList& ethOnChainTransactionsListSync() const = 0;

    /** connext payments transactions, sync interface **/
    virtual Promise<ConnextPaymentList> connextPaymentsList() const = 0;
    virtual const ConnextPaymentList& connextPaymentsListSync() const = 0;;

signals:
    void txnsAdded(std::vector<Transaction> transaction);
    void txnsChanged(std::vector<Transaction> transaction);
    void onChainTxnsAdded(std::vector<OnChainTxRef> transaction);
    void onChainTxnsChanged(std::vector<OnChainTxRef> transaction);
    void onEthChainTxnsAdded(std::vector<EthOnChainTxRef> transaction);
    void onEthChainTxnsChanged(std::vector<EthOnChainTxRef> transaction);
};

//==============================================================================

class AssetsTransactionsCache : public QObject {
    Q_OBJECT
public:
    explicit AssetsTransactionsCache(QObject* parent = nullptr);

    virtual Promise<void> load(bool wipe = false) = 0;
    virtual bool isCreated() const = 0;

    Promise<AbstractTransactionsCache*> cacheById(AssetID assetId);

    virtual AbstractTransactionsCache& cacheByIdSync(AssetID assetId) = 0;
    virtual std::vector<AssetID> availableCaches() const = 0;

signals:
    void cacheAdded(AssetID assetID);

protected:
    QObject* _executionContext{ nullptr };
};

//==============================================================================

#endif // ABSTRACTTRANSACTIONSCACHE_HPP
