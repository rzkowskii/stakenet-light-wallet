#ifndef ACCOUNTSYNCMANAGER_HPP
#define ACCOUNTSYNCMANAGER_HPP

#include <Chain/AbstractChainSyncManager.hpp>
#include <Data/TransactionEntry.hpp>
#include <Networking/AbstractWeb3Client.hpp>
#include <Utils/Utils.hpp>

class WalletDataSource;
class AbstractWeb3Client;
class AccountDataSource;
class AssetsAccountsCache;
class AbstractMutableAccount;
class AbstractChainDataSource;
class AbstractAccountExplorerHttpClient;
class AccountSyncHelper;

class AccountSyncManager : public AbstractChainSyncManager {
    Q_OBJECT
public:
    explicit AccountSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
        CoinAsset asset, AssetsAccountsCache& accountsCache, AccountDataSource& accountDataSource,
        qobject_delete_later_unique_ptr<AbstractWeb3Client> web3Client,
        qobject_delete_later_unique_ptr<AbstractAccountExplorerHttpClient>
            accountExplorerHttpClient,
        std::vector<CoinAsset> activeTokens, QObject* parent = nullptr);

    AbstractMutableAccount& accountsCache() const;

private slots:
    void onAPISyncError(QString error);
    void onTransactionsSynced(QString address, EthOnChainTxList transactions);

    // AbstractChainSyncManager interface
public:
    void trySync();
    void interruptAsync();
    void interrupt();

protected:
    Promise<void> scheduleLastTransactions(QString address, bool isRescan = false);
    Promise<void> schedulePendingTransactions(QString address);
    Promise<void> scheduleFailedTransactions(QString address);
    Promise<Balance> syncAccountBalance(QString address, CoinAsset asset);
    Promise<void> getBestBlockHeight(CoinAsset asset);
    void sync(bool isRescan);

protected:
    AccountDataSource& _accountDataSource;
    AssetsAccountsCache& _accountsCache;
    qobject_delete_later_unique_ptr<AbstractWeb3Client> _web3Client;
    qobject_delete_later_unique_ptr<AbstractAccountExplorerHttpClient> _accountExplorerHttpClient;
    qobject_delete_later_unique_ptr<AccountSyncHelper> _accountSyncHelper;
    std::vector<CoinAsset> _activeTokens;
};

class RescanAccountSyncManager : public AccountSyncManager {
    Q_OBJECT
public:
    explicit RescanAccountSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
        CoinAsset asset, AssetsAccountsCache& accountsCache, AccountDataSource& accountDataSource,
        qobject_delete_later_unique_ptr<AbstractWeb3Client> web3Client,
        qobject_delete_later_unique_ptr<AbstractAccountExplorerHttpClient>
            accountExplorerHttpClient,
        std::vector<CoinAsset> activeTokens, QObject* parent = nullptr);

    void trySync() override;
    void interruptAsync() override;
};

#endif // ACCOUNTSYNCMANAGER_HPP
