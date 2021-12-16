#include "AbstractChainSyncManagerFactory.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Factories/AbstractNetworkingFactory.hpp>

//==============================================================================

AbstractChainSyncManagerFactory::AbstractChainSyncManagerFactory(
    QPointer<WalletAssetsModel> assetsModel, QPointer<AbstractNetworkingFactory> networkingFactory,
    QObject* parent)
    : QObject(parent)
    , _assetsModel(assetsModel)
    , _networkingFactory(networkingFactory)
{
    Q_ASSERT(_assetsModel);
    Q_ASSERT(_networkingFactory);
}

//==============================================================================

AbstractChainSyncManagerFactory::~AbstractChainSyncManagerFactory() {}

//==============================================================================

const WalletAssetsModel* AbstractChainSyncManagerFactory::assetsModel() const
{
    return _assetsModel ? _assetsModel : nullptr;
}

//==============================================================================

AbstractNetworkingFactory* AbstractChainSyncManagerFactory::networkingFactory() const
{
    return _networkingFactory ? _networkingFactory : nullptr;
}

//==============================================================================
