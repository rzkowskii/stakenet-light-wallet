#ifndef TRANSACTIONSCACHE_HPP
#define TRANSACTIONSCACHE_HPP

#include <QObject>
#include <memory>
#include <set>

#include <Chain/AbstractTransactionsCache.hpp>

namespace Utils {
class LevelDBSharedDatabase;
template <class T> class GenericProtoDatabase;
}

//==============================================================================

class AssetTransactionsCacheImpl : public AbstractTransactionsCache {
    Q_OBJECT
public:
    using SaveTxns = std::function<void(const std::vector<Transaction>&)>;
    explicit AssetTransactionsCacheImpl(SaveTxns onSaveTx, QObject* parent = nullptr);
    ~AssetTransactionsCacheImpl() override;

    Promise<TransactionsList> transactionsList() const override;
    Promise<std::vector<QString>> transactionsInBlock(BlockHash blockHash) const override;
    std::vector<QString> transactionsInBlockSync(BlockHash blockHash) const override;

    Promise<void> addTransactions(std::vector<Transaction> transactions) override;
    void addTransactionsSync(std::vector<Transaction> transactions) override;
    Promise<OnChainTxRef> transactionById(QString txId) const override;
    Promise<OnChainTxList> onChainTransactionsList() const override;
    OnChainTxRef transactionByIdSync(QString txId) const override;
    const OnChainTxList& onChainTransactionsListSync() const override;

    Promise<EthOnChainTxList> onEthChainTransactionsList() const override;
    Promise<EthOnChainTxRef> ethTransactionById(QString txId) const override;
    EthOnChainTxRef ethTransactionByIdSync(QString txId) const override;
    const EthOnChainTxList& ethOnChainTransactionsListSync() const override;

    Promise<LightningPaymentList> lnPaymentsList() const override;
    Promise<LightningInvoiceList> lnInvoicesList() const override;
    const LightningPaymentList& lnPaymentsListSync() const override;
    const LightningInvoiceList& lnInvoicesListSync() const override;

    Promise<ConnextPaymentList> connextPaymentsList() const override ;
    const ConnextPaymentList& connextPaymentsListSync() const override;

private:
    chain::TxOutput getOutpointHelper(const chain::TxOutpoint& outpoint);
    void maintainBlockTxIndex(const OnChainTx& tx);

private:
    friend class TransactionsCacheImpl;

    QObject* _executionContext{ nullptr };
    using BlockTransactionsIndex = std::map<BlockHash, std::set<TxID>>;

    OnChainTxList _onChainTransactions;
    LightningPaymentList _lnPayments;
    LightningInvoiceList _lnInvoices;
    EthOnChainTxList _ethOnChainTransactions;
    BlockTransactionsIndex _blockTransactionsIndex;
    ConnextPaymentList _connextPayments;

    SaveTxns _onSaveTx;
};

//==============================================================================

class TransactionsCacheImpl : public AssetsTransactionsCache {
    Q_OBJECT
public:
    explicit TransactionsCacheImpl(
        std::shared_ptr<Utils::LevelDBSharedDatabase> provider, QObject* parent = nullptr);
    Promise<void> load(bool wipe) override;
    bool isCreated() const override;

    AbstractTransactionsCache& cacheByIdSync(AssetID assetId) override;
    std::vector<AssetID> availableCaches() const override;

private:
    void executeLoad(bool wipe);
    void executeSaveTxns(AssetID assetID, const std::vector<Transaction>& txns) const;
    AssetTransactionsCacheImpl& getOrCreateCache(AssetID assetID);

private:
    std::shared_ptr<Utils::LevelDBSharedDatabase> _dbProvider;
    mutable std::map<AssetID, AssetTransactionsCacheImpl*> _caches;
    std::atomic_bool _loaded{ false };
};

//==============================================================================

#endif // TRANSACTIONSCACHE_HPP
