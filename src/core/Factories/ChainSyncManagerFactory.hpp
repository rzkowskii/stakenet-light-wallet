#ifndef CHAINSYNCMANAGERFACTORY_HPP
#define CHAINSYNCMANAGERFACTORY_HPP

#include <Factories/AbstractChainSyncManagerFactory.hpp>
#include <QObject>

namespace Utils {
class WorkerThread;
}

class WalletDataSource;
class AccountDataSource;
class AbstractChainDataSource;
class AssetsTransactionsCache;
class AssetsAccountsCache;
struct BlockFilterMatchable;

class ChainSyncManagerFactory : public AbstractChainSyncManagerFactory {
public:
    explicit ChainSyncManagerFactory(QPointer<WalletAssetsModel> assetsModel,
        QPointer<AbstractNetworkingFactory> networkingFactory, Utils::WorkerThread& workerThread,
        WalletDataSource& dataSource, AccountDataSource& accountDataSource,
        AbstractChainDataSource& chainDataSource, AssetsTransactionsCache& txCache,
        AssetsAccountsCache& accountsCashe, const BlockFilterMatchable& filterMatcher,
        QObject* parent = nullptr);

    AbstractChainSyncManagerPtr createAPISyncManager(Chain& chain) override;
    AbstractChainSyncManagerPtr createRescanSyncManager(Chain& chain) override;

private:
    template <class T> AbstractChainSyncManagerPtr createSyncManager(Chain& chain);
    template <class T> AbstractChainSyncManagerPtr createAccountSyncManager(Chain& chain);

private:
    Utils::WorkerThread& _workerThread;
    WalletDataSource& _dataSource;
    AccountDataSource& _accountDataSource;
    AbstractChainDataSource& _chainDataSource;
    AssetsTransactionsCache& _transactionsCache;
    AssetsAccountsCache& _accountsCashe;
    const BlockFilterMatchable& _filterMatcher;
};

#endif // CHAINSYNCMANAGERFACTORY_HPP
