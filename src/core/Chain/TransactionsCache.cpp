#include "TransactionsCache.hpp"
#include <Tools/DBUtils.hpp>
#include <Utils/GenericProtoDatabase.hpp>
#include <Utils/Logging.hpp>
#include <txdb.h>

#include <QDir>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/range/algorithm_ext.hpp>

//==============================================================================

using namespace boost::adaptors;

//==============================================================================

static const std::string DB_TRANSACTIONS_INDEX{ "transactions_cache" };

//==============================================================================

AssetTransactionsCacheImpl::AssetTransactionsCacheImpl(SaveTxns onSaveTx, QObject* parent)
    : AbstractTransactionsCache(parent)
    , _executionContext(parent)
    , _onSaveTx(onSaveTx)
{
}

//==============================================================================

AssetTransactionsCacheImpl::~AssetTransactionsCacheImpl() {}

//==============================================================================

bool TransactionsCacheImpl::isCreated() const
{
    QDir dir(QString::fromStdString(_dbProvider->path()));
    LogCDebug(General) << "Tx cache data dir:" << dir;
    return dir.exists() && !dir.entryList().isEmpty();
}

//==============================================================================

Promise<TransactionsList> AssetTransactionsCacheImpl::transactionsList() const
{
    return Promise<TransactionsList>([this](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            TransactionsList allTransactions;
            allTransactions.reserve(_onChainTransactions.size() + _lnPayments.size()
                + _lnInvoices.size() + _ethOnChainTransactions.size() + _connextPayments.size());
            auto cpy = [&](const auto& from) {
                std::transform(std::begin(from), std::end(from),
                    std::back_inserter(allTransactions),
                    [](const auto& tx) { return Transaction{ tx }; });
            };

            cpy(_onChainTransactions);
            cpy(_lnPayments);
            cpy(_lnInvoices);
            cpy(_ethOnChainTransactions);
            cpy(_connextPayments);

            resolver(allTransactions);
        });
    });
}

//==============================================================================

Promise<std::vector<QString>> AssetTransactionsCacheImpl::transactionsInBlock(
    BlockHash blockHash) const
{
    return Promise<std::vector<QString>>([=](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(
            _executionContext, [=] { resolver(this->transactionsInBlockSync(blockHash)); });
    });
}

//==============================================================================

std::vector<QString> AssetTransactionsCacheImpl::transactionsInBlockSync(BlockHash blockHash) const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");

    std::vector<QString> result;
    if (_blockTransactionsIndex.count(blockHash) > 0) {
        boost::copy(_blockTransactionsIndex.at(blockHash), std::back_inserter(result));
    }

    return result;
}

//==============================================================================

Promise<void> AssetTransactionsCacheImpl::addTransactions(std::vector<Transaction> transactions)
{
    return Promise<void>([this, transactions](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            this->addTransactionsSync(transactions);
            resolver();
        });
    });
}

//==============================================================================

template <class T> using _Changes = std::array<std::vector<T>, 2>;

//==============================================================================

void AssetTransactionsCacheImpl::addTransactionsSync(std::vector<Transaction> transactions)
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");

    if (transactions.empty()) {
        return;
    }

    _onSaveTx(transactions);

    struct Visitor {
        Visitor(AssetTransactionsCacheImpl* self)
            : self(self)
        {
        }
        void operator()(const OnChainTxRef& tx)
        {
            tx->invalidateLocalCache(std::bind(
                &AssetTransactionsCacheImpl::getOutpointHelper, self, std::placeholders::_1));

            auto it = std::find_if(std::begin(self->_onChainTransactions),
                std::end(self->_onChainTransactions),
                [id = tx->txId()](const auto& ref) { return id == ref->txId(); });

            self->maintainBlockTxIndex(*tx);

            if (it != std::end(self->_onChainTransactions)) {
                *(*it) = *tx;
                onChainTxns.at(0).emplace_back(*it);
                txns.at(0).emplace_back(tx);
            } else {
                self->_onChainTransactions.emplace_back(tx);
                onChainTxns.at(1).emplace_back(tx);
                txns.at(1).emplace_back(tx);
            }
        }

        void operator()(const LightningInvoiceRef& tx)
        {
            auto it = std::lower_bound(std::begin(self->_lnInvoices), std::end(self->_lnInvoices),
                tx->addIndex(),
                [](const auto& lhs, const auto& value) { return lhs->addIndex() < value; });

            if (it != std::end(self->_lnInvoices) && (*it)->addIndex() == tx->addIndex()) {
                *(*it) = *tx;
                txns.at(0).emplace_back(*it);
            } else {
                self->_lnInvoices.emplace_back(tx);
                txns.at(1).emplace_back(tx);
            }
        }

        void operator()(const LightningPaymentRef& tx)
        {
            auto it = std::lower_bound(std::begin(self->_lnPayments), std::end(self->_lnPayments),
                tx->paymentIndex(),
                [](const auto& lhs, const auto& value) { return lhs->paymentIndex() < value; });

            if (it != std::end(self->_lnPayments)) {
                *(*it) = *tx;
                txns.at(0).emplace_back(*it);
            } else {
                self->_lnPayments.emplace_back(tx);
                txns.at(1).emplace_back(tx);
            }
        }

        void operator()(const EthOnChainTxRef& tx)
        {
            auto it = std::lower_bound(std::begin(self->_ethOnChainTransactions), std::end(self->_ethOnChainTransactions),
                tx->nonce(),
                [](const auto& lhs, const auto& value) { return lhs->nonce() < value; });

            if (it != std::end(self->_ethOnChainTransactions) && it->get()->nonce() == tx->nonce()) {
                *(*it) = *tx;
                ethOnChainTxns.at(0).emplace_back(*it);
                txns.at(0).emplace_back(tx);
            } else {
                self->_ethOnChainTransactions.emplace_back(tx);
                ethOnChainTxns.at(1).emplace_back(tx);
                txns.at(1).emplace_back(tx);
            }
        }

        void operator()(const ConnextPaymentRef& tx)
        {
            auto it = std::find_if(std::begin(self->_connextPayments), std::end(self->_connextPayments),
                [id = tx->transferId()](const auto& ref) { return ref->transferId() == id; });

            if (it != std::end(self->_connextPayments)) {
                *(*it) = *tx;
                txns.at(0).emplace_back(*it);
            } else {
                self->_connextPayments.emplace_back(tx);
                txns.at(1).emplace_back(tx);
            }
        }

        void postProcess()
        {

            auto process = [](const auto& txns, auto onChanged, auto onAdded) {
                if (!txns.at(0).empty()) {
                    onChanged(txns.at(0));
                }
                if (!txns.at(1).empty()) {
                    onAdded(txns.at(1));
                }
            };

            process(onChainTxns,
                std::bind(
                    &AssetTransactionsCacheImpl::onChainTxnsChanged, self, std::placeholders::_1),
                std::bind(
                    &AssetTransactionsCacheImpl::onChainTxnsAdded, self, std::placeholders::_1));

            process(ethOnChainTxns,
                std::bind(&AssetTransactionsCacheImpl::onEthChainTxnsChanged, self,
                    std::placeholders::_1),
                std::bind(
                    &AssetTransactionsCacheImpl::onEthChainTxnsAdded, self, std::placeholders::_1));

            process(txns,
                std::bind(&AssetTransactionsCacheImpl::txnsChanged, self, std::placeholders::_1),
                std::bind(&AssetTransactionsCacheImpl::txnsAdded, self, std::placeholders::_1));
        }

        // _Changes is an array of vector of events that will be delivered
        // to user, used to deliver added or changed.
        _Changes<Transaction> txns;
        _Changes<OnChainTxRef> onChainTxns;
        _Changes<LightningPaymentRef> lnPaymentsTxns;
        _Changes<LightningInvoiceRef> lnInvoicesTxns;
        _Changes<EthOnChainTxRef> ethOnChainTxns;
        _Changes<ConnextPaymentRef> connextPaymentsTxns;
        AssetTransactionsCacheImpl* self;
    };

    Visitor visitor(this);

    for (const auto& tx : transactions) {
        boost::variant2::visit(visitor, tx);
    }

    visitor.postProcess();
}

//==============================================================================

Promise<OnChainTxRef> AssetTransactionsCacheImpl::transactionById(QString txId) const
{
    return Promise<OnChainTxRef>([=](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto tx = TransactionUtils::FindTransaction(_onChainTransactions, txId);
            tx ? resolver(tx) : reject(nullptr);
        });
    });
}

//==============================================================================

OnChainTxRef AssetTransactionsCacheImpl::transactionByIdSync(QString txId) const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return TransactionUtils::FindTransaction(_onChainTransactions, txId);
}

//==============================================================================

const OnChainTxList& AssetTransactionsCacheImpl::onChainTransactionsListSync() const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return _onChainTransactions;
}

//==============================================================================

Promise<EthOnChainTxList> AssetTransactionsCacheImpl::onEthChainTransactionsList() const
{
    return Promise<EthOnChainTxList>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_ethOnChainTransactions); });
    });
}

//==============================================================================

Promise<EthOnChainTxRef> AssetTransactionsCacheImpl::ethTransactionById(QString txId) const
{
    return Promise<EthOnChainTxRef>([=](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto tx = TransactionUtils::FindEthTransaction(_ethOnChainTransactions, txId);
            tx ? resolver(tx) : reject(nullptr);
        });
    });
}

//==============================================================================

EthOnChainTxRef AssetTransactionsCacheImpl::ethTransactionByIdSync(QString txId) const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return TransactionUtils::FindEthTransaction(_ethOnChainTransactions, txId);
    ;
}

//==============================================================================

const EthOnChainTxList& AssetTransactionsCacheImpl::ethOnChainTransactionsListSync() const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return _ethOnChainTransactions;
}

//==============================================================================

Promise<LightningPaymentList> AssetTransactionsCacheImpl::lnPaymentsList() const
{
    return Promise<LightningPaymentList>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_lnPayments); });
    });
}

//==============================================================================

Promise<LightningInvoiceList> AssetTransactionsCacheImpl::lnInvoicesList() const
{
    return Promise<LightningInvoiceList>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_lnInvoices); });
    });
}

//==============================================================================

const LightningPaymentList& AssetTransactionsCacheImpl::lnPaymentsListSync() const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return _lnPayments;
}

//==============================================================================

const LightningInvoiceList& AssetTransactionsCacheImpl::lnInvoicesListSync() const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return _lnInvoices;
}

//==============================================================================

Promise<ConnextPaymentList> AssetTransactionsCacheImpl::connextPaymentsList() const
{
    return Promise<ConnextPaymentList>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_connextPayments); });
    });
}

//==============================================================================

const ConnextPaymentList &AssetTransactionsCacheImpl::connextPaymentsListSync() const
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return _connextPayments;
}

//==============================================================================

void TransactionsCacheImpl::executeLoad(bool wipe)
{
    if (!_loaded) {
        _dbProvider->registerIndex(DB_TRANSACTIONS_INDEX, DB_TRANSACTIONS_INDEX);
        using TxDB = Utils::GenericProtoDatabase<chain::Transaction>;
        TxDB::Cache cache;

        auto load = [&] {
            using namespace bitcoin;
            std::unique_ptr<CDBIterator> pcursor(_dbProvider->NewIterator());
            std::pair<std::string, std::pair<AssetID, std::string>> key;
            pcursor->Seek(DB_TRANSACTIONS_INDEX);

            while (pcursor->Valid() && pcursor->GetKey(key) && key.first == DB_TRANSACTIONS_INDEX) {
                std::vector<unsigned char> serialization;
                if (pcursor->GetValue(serialization)) {

                    chain::Transaction tx;
                    if (tx.ParseFromArray(serialization.data(), serialization.size())) {
                        auto assetID = tx.asset_id();

                        switch (tx.transaction_case()) {
                        case chain::Transaction::kOnchainTx: {
                            this->getOrCreateCache(assetID)._onChainTransactions.emplace_back(
                                std::make_shared<OnChainTx>(tx));
                            break;
                        }
                        case chain::Transaction::kLightningPayment: {
                            this->getOrCreateCache(assetID)._lnPayments.emplace_back(
                                std::make_shared<LightningPayment>(tx));
                            break;
                        }
                        case chain::Transaction::kLightningInvoice: {
                            this->getOrCreateCache(assetID)._lnInvoices.emplace_back(
                                std::make_shared<LightningInvoice>(tx));
                            break;
                        }
                        case chain::Transaction::kEthonchainTx: {
                            this->getOrCreateCache(assetID)._ethOnChainTransactions.emplace_back(
                                std::make_shared<EthOnChainTx>(tx));
                            break;
                        }
                        case chain::Transaction::kConnextPayment: {
                            this->getOrCreateCache(assetID)._connextPayments.emplace_back(
                                std::make_shared<ConnextPayment>(tx));
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    pcursor->Next();
                } else {
                }
            }
        };

        load();

        for (auto&& it : _caches) {
            auto& cache = *it.second;
            std::sort(std::begin(cache._onChainTransactions), std::end(cache._onChainTransactions),
                [](const auto& lhs, const auto& rhs) {
                    // if comparing with pending tx
                    if (lhs->blockHeight() < 0 || rhs->blockHeight() < 0) {
                        return lhs->tx().timestamp() < rhs->tx().timestamp();
                    } else {
                        return lhs->blockHeight() == rhs->blockHeight()
                            ? lhs->transactionIndex() < rhs->transactionIndex()
                            : lhs->blockHeight() < rhs->blockHeight();
                    }
                });

            std::sort(std::begin(cache._ethOnChainTransactions),
                std::end(cache._ethOnChainTransactions), [](const auto& lhs, const auto& rhs) {
                    // sort all transactions using nonce, eth wants a strict sequential ordering of
                    // nonces so we should be good if we have everything sorted this way.
                    return lhs->nonce() < rhs->nonce();
                });

            for (auto&& tx : cache._onChainTransactions) {
                tx->invalidateLocalCache(std::bind(
                    &AssetTransactionsCacheImpl::getOutpointHelper, &cache, std::placeholders::_1));
                cache.maintainBlockTxIndex(*tx);
            }

            std::sort(std::begin(cache._lnPayments), std::end(cache._lnPayments),
                [](const auto& lhs, const auto& rhs) {
                    return lhs->paymentIndex() < rhs->paymentIndex();
                });

            std::sort(std::begin(cache._lnInvoices), std::end(cache._lnInvoices),
                [](const auto& lhs, const auto& rhs) { return lhs->addIndex() < rhs->addIndex(); });

            std::sort(std::begin(cache._connextPayments), std::end(cache._connextPayments),
                [](const auto& lhs, const auto& rhs) { return lhs->transactionDate() < rhs->transactionDate(); });
        }

        _loaded = true;
        LogCDebug(Chains) << "TransactionsCache loaded";
    }
}

//==============================================================================

void TransactionsCacheImpl::executeSaveTxns(
    AssetID assetID, const std::vector<Transaction>& txns) const
{
    struct Visitor {
        using Result = std::tuple<std::string, std::vector<unsigned char>>;
        Result operator()(const OnChainTxRef& tx)
        {
            return std::make_tuple(tx->tx().id(), ProtobufSerializeToArray(tx->_tx));
        }

        Result operator()(const LightningInvoiceRef& tx)
        {
            auto id = std::string("i_") + std::to_string(tx->addIndex());
            return std::make_tuple(id, ProtobufSerializeToArray(tx->_tx));
        }

        Result operator()(const LightningPaymentRef& tx)
        {
            auto id = std::string("p_") + std::to_string(tx->paymentIndex());
            return std::make_tuple(id, ProtobufSerializeToArray(tx->_tx));
        }
        Result operator()(const EthOnChainTxRef& tx)
        {
            return std::make_tuple(tx->tx().id(), ProtobufSerializeToArray(tx->_tx));
        }
        Result operator()(const ConnextPaymentRef& tx)
        {
            auto id = std::string("cp_") + tx->transferId().toStdString();
            return std::make_tuple(id, ProtobufSerializeToArray(tx->_tx));
        }

    };

    bitcoin::CDBBatch batch(*_dbProvider);
    std::string id;
    std::vector<unsigned char> serialized;

    for (auto&& tx : txns) {
        std::tie(id, serialized) = boost::variant2::visit(Visitor{}, tx);
        batch.Write(std::make_pair(DB_TRANSACTIONS_INDEX, std::make_pair(assetID, id)), serialized);
    }

    _dbProvider->WriteBatch(batch, true);
}

//==============================================================================

AssetTransactionsCacheImpl& TransactionsCacheImpl::getOrCreateCache(AssetID assetID)
{
    if (_caches.count(assetID) == 0) {

        AssetTransactionsCacheImpl::SaveTxns onSaveTx = std::bind(
            &TransactionsCacheImpl::executeSaveTxns, this, assetID, std::placeholders::_1);
        _caches.emplace(assetID, new AssetTransactionsCacheImpl(onSaveTx, _executionContext));
        cacheAdded(assetID);
    }

    return *_caches.at(assetID);
}

//==============================================================================

chain::TxOutput AssetTransactionsCacheImpl::getOutpointHelper(const chain::TxOutpoint& outpoint)
{
    chain::TxOutput result;

    if (auto tx = TransactionUtils::FindTransaction(
            _onChainTransactions, QString::fromStdString(outpoint.hash()))) {
        result = TransactionUtils::FindOutput(*tx, outpoint.index()).get_value_or(result);
    }

    return result;
}

//==============================================================================

void AssetTransactionsCacheImpl::maintainBlockTxIndex(const OnChainTx& tx)
{
    if (!tx.blockHash().isEmpty()) {
        _blockTransactionsIndex[tx.blockHash()].emplace(tx.txId());
    }
}

//==============================================================================

Promise<OnChainTxList> AssetTransactionsCacheImpl::onChainTransactionsList() const
{
    return Promise<OnChainTxList>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_onChainTransactions); });
    });
}

//==============================================================================

TransactionsCacheImpl::TransactionsCacheImpl(
    std::shared_ptr<Utils::LevelDBSharedDatabase> provider, QObject* parent)
    : AssetsTransactionsCache(parent)
    , _dbProvider(provider)
{
}

//==============================================================================

Promise<void> TransactionsCacheImpl::load(bool wipe)
{
    return Promise<void>([=](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                this->executeLoad(wipe);
                resolver();
            } catch (std::exception& ex) {
                LogCCritical(Chains) << "Failed to load tx cache:" << ex.what();
                reject(ex);
            }
        });
    });
}

//==============================================================================

AbstractTransactionsCache& TransactionsCacheImpl::cacheByIdSync(AssetID assetId)
{
    Q_ASSERT_X(thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");
    return getOrCreateCache(assetId);
}

//==============================================================================

std::vector<AssetID> TransactionsCacheImpl::availableCaches() const
{
    std::vector<AssetID> ids;
    for (auto&& it : _caches) {
        ids.emplace_back(it.first);
    }

    return ids;
}

//==============================================================================
