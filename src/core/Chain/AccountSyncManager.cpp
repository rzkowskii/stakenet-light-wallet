#include "AccountSyncManager.hpp"
#include <Chain/AbstractAssetAccount.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/AccountCacheImpl.hpp>
#include <Chain/AccountSyncHelper.hpp>
#include <Chain/Chain.hpp>
#include <EthCore/Encodings.hpp>
#include <EthCore/Types.hpp>
#include <Models/WalletDataSource.hpp>
#include <Networking/AbstractAccountExplorerHttpClient.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Networking/Web3Client.hpp>
#include <Tools/Common.hpp>

//==============================================================================

AccountSyncManager::AccountSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
    CoinAsset asset, AssetsAccountsCache& accountsCache, AccountDataSource& accountDataSource,
    qobject_delete_later_unique_ptr<AbstractWeb3Client> web3Client,
    qobject_delete_later_unique_ptr<AbstractAccountExplorerHttpClient> accountExplorerHttpClient,
    std::vector<CoinAsset> activeTokens, QObject* parent)
    : AbstractChainSyncManager(chain, transactionsCache, asset, parent)
    , _accountDataSource(accountDataSource)
    , _accountsCache(accountsCache)
    , _web3Client(std::move(web3Client))
    , _accountExplorerHttpClient(std::move(accountExplorerHttpClient))
    , _accountSyncHelper(
          new AccountSyncHelper(_accountExplorerHttpClient.get(), asset, activeTokens))
    , _activeTokens(activeTokens)
{
    _activeTokens.emplace_back(coinAsset());
}

//==============================================================================

AbstractMutableAccount& AccountSyncManager::accountsCache() const
{
    return _accountsCache.mutableCacheByIdSync(coinAsset().coinID());
}

//==============================================================================

void AccountSyncManager::onAPISyncError(QString error)
{
    LogCCritical(Sync) << "API Sync error" << error;
    setIsSyncing(false);
    syncError(error);
}

//==============================================================================

void AccountSyncManager::onTransactionsSynced(QString address, EthOnChainTxList transactions)
{
    if (!isSyncing()) {
        return;
    }

    if (!transactions.empty()) {
        std::map<AssetID, std::vector<Transaction>> parsedTransactions;

        for (auto&& tx : transactions) {
            auto isTransferPayload
                = QString::fromStdString(tx->input())
                      .startsWith(
                          eth::erc20::transferPayload(address.toLower(), tx->value()).left(10));

            if (tx->isBaseChainTx() || isTransferPayload) {
                auto txAssetID = tx->assetID();
                auto transaction = Transaction{ TransactionUtils::UpdateEthTransactionType(
                    address, isTransferPayload, tx) };

                if (parsedTransactions.count(txAssetID)) {
                    parsedTransactions.at(txAssetID).push_back(transaction);
                } else {
                    parsedTransactions[txAssetID] = std::vector<Transaction>{ transaction };
                }
            }
        }

        for (auto &&pair : parsedTransactions) {
            assetsTxCache().cacheByIdSync(pair.first).addTransactionsSync(pair.second);
        }

        // syncProgress(0);
    } else {
        LogCDebug(Sync) << "Got sequence of transaction with unknown prevTxHash";
        // Q_ASSERT(_headersProcessingQueue.empty()); // we should get here only if we don't have
        // any work left
        setIsSyncing(false);
    }
}

//==============================================================================

void AccountSyncManager::trySync()
{
    sync(false);
}

//==============================================================================

void AccountSyncManager::interruptAsync()
{
    Utils::ScheduleJob(
        this, std::bind(&AccountSyncManager::setIsSyncing, std::placeholders::_1, false));
    LogCDebug(Sync) << "AccountSyncManager task interrupted";
}

//==============================================================================

void AccountSyncManager::interrupt()
{
    interruptAsync();
}

//==============================================================================

Promise<void> AccountSyncManager::scheduleLastTransactions(QString address, bool isRescan)
{
    if (coinAsset().token()) {
        return Promise<void>::resolve();
    }

    const auto& txns = txCache().ethOnChainTransactionsListSync();
    auto lastTxHash = !txns.empty() && !isRescan ? txns.back()->txId() : QString{};

    return _accountSyncHelper->getTransactions(address, lastTxHash)
        .then([this, address](
                  EthOnChainTxList transactions) { onTransactionsSynced(address, transactions); })
        .then([this, address] { return scheduleFailedTransactions(address); })
        .then([this, address] { return schedulePendingTransactions(address); })
        .tapFail([this](const NetworkUtils::ApiErrorException& error) {
            onAPISyncError(error.errorResponse);
        });
}

//==============================================================================

Promise<void> AccountSyncManager::schedulePendingTransactions(QString address)
{
    // TODO: Yurii - need to handle missing timestamp in web3 api where placing pending tx
    if (coinAsset().token()) {
        return Promise<void>::resolve();
    }

    const auto& txns = txCache().ethOnChainTransactionsListSync();
    std::vector<EthOnChainTxRef> pendingTxns;
    for (auto it = txns.rbegin(); it != txns.rend(); ++it) {
        auto tx = *it;
        if (tx->blockHash() == EthOnChainTx::UNKNOWN_BLOCK_HASH
            || tx->blockHeight() == EthOnChainTx::UNKNOWN_BLOCK_HEIGHT) {
            pendingTxns.emplace_back(tx);
        }
    }

    return QtPromise::map(pendingTxns,
        [this, address](const EthOnChainTxRef& tx, ...) {
            return _web3Client->getTransactionByHash(tx->txId()).then([tx](QVariantMap obj) {
                return std::make_tuple(tx, obj);
            });
        })
        .filter([](std::tuple<EthOnChainTxRef, QVariantMap> t, ...) {
            return AccountSyncHelper::IsRemoteTxConfirmed(std::get<1>(t));
        })
        .map([address](std::tuple<EthOnChainTxRef, QVariantMap> t, ...) {
            auto [tx, obj] = t;
            auto mergedTx = AccountSyncHelper::MergeTransactions(tx, obj);
            return TransactionUtils::UpdateEthTransactionStatus(mergedTx);
        })
        .then([this](QVector<EthOnChainTxRef> txns) {
            txCache().addTransactionsSync({ txns.begin(), txns.end() });
        });
}

//==============================================================================

Promise<void> AccountSyncManager::scheduleFailedTransactions(QString address)
{
    if (coinAsset().token()) {
        return Promise<void>::resolve();
    }

    const auto& txns = txCache().ethOnChainTransactionsListSync();
    std::vector<EthOnChainTxRef> notConfirmedTxns;
    for (auto it = txns.rbegin(); it != txns.rend(); ++it) {
        auto tx = *it;
        if (tx->blockHash() == EthOnChainTx::UNKNOWN_BLOCK_HASH
            || tx->blockHeight() == EthOnChainTx::UNKNOWN_BLOCK_HEIGHT) {
            notConfirmedTxns.emplace_back(tx);
        }
    }

    return QtPromise::map(notConfirmedTxns,
        [this, address](const EthOnChainTxRef& tx, ...) {
            return _web3Client->getTransactionByHash(tx->txId()).then([tx](QVariantMap obj) {
                return std::make_tuple(tx, obj);
            });
        })
        .filter([](std::tuple<EthOnChainTxRef, QVariantMap> t, ...) {
            const auto& [_, obj] = t;
            return !obj.empty()
                && obj.value("status")
                == "0x0"; // status = "0x0" - tx failed, status = "0x1" - tx success
        })
        .map([address](std::tuple<EthOnChainTxRef, QVariantMap> t, ...) {
            auto [tx, obj] = t;
            auto mergedTx = AccountSyncHelper::MergeTransactions(tx, obj);
            return TransactionUtils::UpdateEthTransactionStatus(mergedTx, "0x0");
        })
        .then([this](QVector<EthOnChainTxRef> txns) {
            txCache().addTransactionsSync({ txns.begin(), txns.end() });
        });
}

//==============================================================================

Promise<Balance> AccountSyncManager::syncAccountBalance(QString address, CoinAsset asset)
{
    if (auto token = asset.token()) {
        QVariantMap params;
        params["to"] = token->contract();
        params["data"] = eth::erc20::balanceOfPayload(address);
        return _web3Client->call(params)
            .then([decimals = token->decimals()](QString result) {
                return eth::ConvertFromWeiToSats(eth::ConvertFromHex(result), decimals, 8);
            })
            .tapFail([](std::exception& ex) {
                qDebug() << "Failed to fetch token balance:" << ex.what();
            });
    } else {
        return _web3Client->getBalance(address)
            .then([](eth::u256 balanceReceived) {
                return eth::ConvertFromWeiToSats(balanceReceived);
            })
            .tapFail([](std::exception& ex) {
                qDebug() << "Failed to fetch account balance:" << ex.what();
            });
    }
}

//==============================================================================

Promise<void> AccountSyncManager::getBestBlockHeight(CoinAsset asset)
{
    if (coinAsset().token()) {
        return Promise<void>::resolve();
    }

    return Promise<void>([=](const auto& resolve, const auto& reject) {
        _web3Client->getBlockNumber()
            .then([this, asset, resolve, reject](eth::u64 bestBlockHeight) {
                auto blockNumber
                    = QString::fromStdString(eth::ConvertFromDecimalToHex(bestBlockHeight));
                _web3Client->getBlockByNumber(blockNumber)
                    .then([this, resolve, bestBlockHeight](QVariantMap blockInfo) {
                        chain().setEthChain(blockInfo.value("hash").toString(),
                            bestBlockHeight.convert_to<int64_t>());
                        resolve();
                    })
                    .tapFail([reject](std::exception& ex) {
                        qDebug() << "Failed to fetch block hash:" << ex.what();
                        reject(std::current_exception());
                    });
            })
            .tapFail([reject](std::exception& ex) {
                qDebug() << "Failed to fetch block number:" << ex.what();
                reject(std::current_exception());
            });
    });
}

//==============================================================================

void AccountSyncManager::sync(bool isRescan)
{
    setIsSyncing(true);
    // TODO(yuraolex): add check if it's connected or not, remove this later.
    QTimer::singleShot(5000, this, [this, isRescan] {
        _accountDataSource.getAccountAddress(coinAsset().coinID())
            .then([this, isRescan](QString address) {
                // address = "0xfe47fc0d8dcC526968511084E9Df04F7dC4952be";
                // address = "0xba8e7E613346d52f8121c7cdbf35b9Fb12E77d16";
                // address = "0x81B4E2dC68F0201291e2904a751b46e3B3561017"; // shahab
                getBestBlockHeight(coinAsset()).then([this, address, isRescan]() {
                    auto tokensSyncPromise = QtPromise::map(_activeTokens,
                        [this, address](const CoinAsset& asset, ...) {
                            return syncAccountBalance(address, asset)
                                .then([this, assetId = asset.coinID()](Balance newBalance) {
                                    _accountsCache.mutableCacheByIdSync(assetId).setAccountBalance(
                                        newBalance);
                                    return true;
                                });
                        })
                                                 .then([](const QVector<bool>&) {});

                    auto txSyncPromise = scheduleLastTransactions(address, isRescan);
                    QtPromise::all(std::vector<Promise<void>>{ tokensSyncPromise, txSyncPromise })
                        .finally([this] { this->setIsSyncing(false); });
                });
            });
    });
}

//==============================================================================

RescanAccountSyncManager::RescanAccountSyncManager(Chain& chain,
    AssetsTransactionsCache& transactionsCache, CoinAsset asset, AssetsAccountsCache& accountsCache,
    AccountDataSource& accountDataSource,
    qobject_delete_later_unique_ptr<AbstractWeb3Client> web3Client,
    qobject_delete_later_unique_ptr<AbstractAccountExplorerHttpClient> accountExplorerHttpClient,
    std::vector<CoinAsset> activeTokens, QObject* parent)
    : AccountSyncManager(chain, transactionsCache, asset, accountsCache, accountDataSource,
          std::move(web3Client), std::move(accountExplorerHttpClient), activeTokens, parent)
{
}

//==============================================================================

void RescanAccountSyncManager::trySync()
{
    sync(true);
}

//==============================================================================

void RescanAccountSyncManager::interruptAsync()
{
    setIsSyncing(false);
    LogCDebug(Sync) << "RescanAccountSyncManager task interrupted";
}

//==============================================================================
