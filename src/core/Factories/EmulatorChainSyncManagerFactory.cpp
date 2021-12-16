#include "EmulatorChainSyncManagerFactory.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/Chain.hpp>
#include <Chain/EmulatorChainSyncManager.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/AbstractNetworkingFactory.hpp>

//==============================================================================

EmulatorChainSyncManagerFactory::EmulatorChainSyncManagerFactory(
    QPointer<WalletAssetsModel> assetsModel, QPointer<AbstractNetworkingFactory> networkingFactory,
    Utils::WorkerThread& workerThread, WalletDataSource& dataSource,
    AssetsTransactionsCache& txCache, const BlockFilterMatchable& filterMatcher,
    EmulatorViewModel& emulatorViewModel, QObject* parent)
    : AbstractChainSyncManagerFactory(assetsModel, networkingFactory, parent)
    , _workerThread(workerThread)
    , _dataSource(dataSource)
    , _transactionsCache(txCache)
    , _filterMatcher(filterMatcher)
    , _emulatorViewModel(emulatorViewModel)
{
}

//==============================================================================

AbstractChainSyncManagerPtr EmulatorChainSyncManagerFactory::createAPISyncManager(Chain& chain)
{
    return createSyncManager<EmulatorChainSyncManager>(chain);
}

//==============================================================================

AbstractChainSyncManagerPtr EmulatorChainSyncManagerFactory::createRescanSyncManager(Chain& chain)
{
    return createSyncManager<EmulatorChainSyncManager>(chain);
}

//==============================================================================

template <class T>
AbstractChainSyncManagerPtr EmulatorChainSyncManagerFactory::createSyncManager(Chain& chain)
{
    AbstractChainSyncManagerPtr chainSyncManager;
    if (assetsModel() && networkingFactory()) {
        auto apiClient = networkingFactory()->createBlockExplorerClient(chain.assetID());
        chainSyncManager.reset(new T(chain, _transactionsCache, _dataSource, _filterMatcher,
            assetsModel()->assetById(chain.assetID()), _workerThread, apiClient.release(),
            _emulatorViewModel));

        chainSyncManager->moveToThread(&_workerThread);
    }

    return chainSyncManager;
}

//==============================================================================
