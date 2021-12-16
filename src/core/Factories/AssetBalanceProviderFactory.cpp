#include "AssetBalanceProviderFactory.hpp"

#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/ChainManager.hpp>
#include <Data/AssetAccountBalanceProvider.hpp>
#include <Data/AssetUTXOBalanceProvider.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Models/LnDaemonInterface.hpp>
#include <Models/ConnextDaemonInterface.hpp>

//==============================================================================

AssetBalanceProviderFactory::AssetBalanceProviderFactory(
    QPointer<AssetsTransactionsCache> assetsTxnsCache, QPointer<AssetsAccountsCache> accountsCache, QPointer<AbstractChainManager> chainManager,
    QPointer<PaymentNodesManager> nodeManager)
    : _assetsTxnsCache(assetsTxnsCache)
    , _accountsCache(accountsCache)
    , _chainManager(chainManager)
    , _nodeManager(nodeManager)
{
}

//==============================================================================

std::unique_ptr<AbstractAssetBalanceProvider> AssetBalanceProviderFactory::createBalanceProvider(
    CoinAsset asset)
{
    auto nodeInterface = _nodeManager->interfaceById(asset.coinID());
    switch (asset.type()) {
    case CoinAsset::Type::UTXO:
        return std::make_unique<AssetUTXOBalanceProvider>(
            asset, _assetsTxnsCache, _chainManager, qobject_cast<LnDaemonInterface*>(nodeInterface));
    case CoinAsset::Type::Account:
        return std::make_unique<AssetAccountBalanceProvider>(asset, _accountsCache, qobject_cast<ConnextDaemonInterface*>(nodeInterface));
    default:
        throw std::runtime_error("Not supported account type");
    }

    return {};
}

//==============================================================================
