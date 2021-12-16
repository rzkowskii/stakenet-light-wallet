#include "PaymentNodesManager.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/WalletDataSource.hpp>

#include <Utils/Utils.hpp>

//==============================================================================

PaymentNodesManager::PaymentNodesManager(QPointer<Utils::WorkerThread> workerThread,
    QPointer<AssetsTransactionsCache> txCache, QPointer<WalletAssetsModel> assetsModel,
    AccountDataSource& dataSource, AbstractNetworkingFactory &apiClientFactory, QObject* parent)
    : QObject(parent)
{
    _lnDaemonManager.reset(new LnDaemonsManager(*workerThread, *txCache, *assetsModel));
    _connextDaemonsManager.reset(
        new ConnextDaemonsManager(*workerThread, *assetsModel, dataSource, apiClientFactory, txCache));
}

//==============================================================================

PaymentNodesManager::~PaymentNodesManager() {}

//==============================================================================

PaymentNodeInterface* PaymentNodesManager::interfaceById(AssetID assetID) const
{
    if (auto r = _lnDaemonManager->interfaceById(assetID)) {
        return r;
    }

    return _connextDaemonsManager->interfaceById(assetID);
}

//==============================================================================

QPointer<LnDaemonsManager> PaymentNodesManager::lnDaemonManager() const
{
    return _lnDaemonManager.get();
}

//==============================================================================

QPointer<ConnextDaemonsManager> PaymentNodesManager::connextDaemonManager() const
{
    return _connextDaemonsManager.get();
}

//==============================================================================
