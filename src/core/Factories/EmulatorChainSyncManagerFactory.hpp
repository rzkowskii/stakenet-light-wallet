#ifndef EMULATORCHAINSYNCMANAGERFACTORY_HPP
#define EMULATORCHAINSYNCMANAGERFACTORY_HPP

#include <Factories/AbstractChainSyncManagerFactory.hpp>
#include <QObject>

namespace Utils {
class WorkerThread;
}

class WalletDataSource;
class AssetsTransactionsCache;
class EmulatorViewModel;
struct BlockFilterMatchable;

class EmulatorChainSyncManagerFactory : public AbstractChainSyncManagerFactory {
    Q_OBJECT
public:
    explicit EmulatorChainSyncManagerFactory(QPointer<WalletAssetsModel> assetsModel,
        QPointer<AbstractNetworkingFactory> networkingFactory, Utils::WorkerThread& workerThread,
        WalletDataSource& dataSource, AssetsTransactionsCache& txCache,
        const BlockFilterMatchable& filterMatcher, EmulatorViewModel& emulatorViewModel,
        QObject* parent = nullptr);

public:
    AbstractChainSyncManagerPtr createAPISyncManager(Chain& chain) override;
    AbstractChainSyncManagerPtr createRescanSyncManager(Chain& chain) override;

private:
    template <class T> AbstractChainSyncManagerPtr createSyncManager(Chain& chain);

private:
    Utils::WorkerThread& _workerThread;
    WalletDataSource& _dataSource;
    AssetsTransactionsCache& _transactionsCache;
    const BlockFilterMatchable& _filterMatcher;
    EmulatorViewModel& _emulatorViewModel;
};

#endif // EMULATORCHAINSYNCMANAGERFACTORY_HPP
