#include "ChainSyncManagerFactory.hpp"

#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/AccountSyncManager.hpp>
#include <Chain/Chain.hpp>
#include <Chain/ChainSyncManager.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/AbstractNetworkingFactory.hpp>
#include <Models/WalletDataSource.hpp>
#include <Networking/AbstractAccountExplorerHttpClient.hpp>
#include <Networking/AbstractBlockExplorerHttpClient.hpp>

#include <QNetworkAccessManager>
#include <QNetworkReply>

//==============================================================================

ChainSyncManagerFactory::ChainSyncManagerFactory(QPointer<WalletAssetsModel> assetsModel,
    QPointer<AbstractNetworkingFactory> networkingFactory, Utils::WorkerThread& workerThread,
    WalletDataSource& dataSource, AccountDataSource& accountDataSource,
    AbstractChainDataSource& chainDataSource, AssetsTransactionsCache& txCache,
    AssetsAccountsCache& accountsCashe, const BlockFilterMatchable& filterMatcher, QObject* parent)
    : AbstractChainSyncManagerFactory(assetsModel, networkingFactory, parent)
    , _workerThread(workerThread)
    , _dataSource(dataSource)
    , _accountDataSource(accountDataSource)
    , _chainDataSource(chainDataSource)
    , _transactionsCache(txCache)
    , _accountsCashe(accountsCashe)
    , _filterMatcher(filterMatcher)
{
}

//==============================================================================

AbstractChainSyncManagerPtr ChainSyncManagerFactory::createAPISyncManager(Chain& chain)
{
    switch (assetsModel()->assetById(chain.assetID()).type()) {
    case CoinAsset::Type::UTXO:
        return createSyncManager<ChainSyncManager>(chain);
    case CoinAsset::Type::Account:
        return createAccountSyncManager<AccountSyncManager>(chain);
    default:
        break;
    }

    Q_ASSERT_X(false, __FUNCTION__, "Invalid asset type");

    return {};
}

//==============================================================================

AbstractChainSyncManagerPtr ChainSyncManagerFactory::createRescanSyncManager(Chain& chain)
{
    switch (assetsModel()->assetById(chain.assetID()).type()) {
    case CoinAsset::Type::UTXO:
        return createSyncManager<RescanSyncManager>(chain);
    case CoinAsset::Type::Account:
        return createAccountSyncManager<RescanAccountSyncManager>(chain);
    default:
        break;
    }

    Q_ASSERT_X(false, __FUNCTION__, "Invalid asset type");

    return {};
}

//==============================================================================

template <class T>
AbstractChainSyncManagerPtr ChainSyncManagerFactory::createSyncManager(Chain& chain)
{
    AbstractChainSyncManagerPtr chainSyncManager;
    if (assetsModel() && networkingFactory()) {
        //        auto apiClient = networkingFactory()->createBlockExplorerClient(chain.assetID());
        chainSyncManager.reset(new T(chain, _transactionsCache, _dataSource, _filterMatcher,
            assetsModel()->assetById(chain.assetID()), _chainDataSource, nullptr));

        chainSyncManager->moveToThread(&_workerThread);
    }

    return chainSyncManager;
}

//==============================================================================

template <class T>
AbstractChainSyncManagerPtr ChainSyncManagerFactory::createAccountSyncManager(Chain& chain)
{
    AbstractChainSyncManagerPtr chainSyncManager;
    if (assetsModel() && networkingFactory()) {
        auto asset = assetsModel()->assetById(chain.assetID());
        auto web3Client = networkingFactory()->createWeb3Client(*asset.params().chainId);
        auto httpExplorerClient = networkingFactory()->createAccountExplorerClient(asset.coinID());

        std::vector<CoinAsset> activeTokens;
        for (auto&& id : assetsModel()->activeAssets()) {
            auto tokenAsset = assetsModel()->assetById(id);
            if (tokenAsset.token() && tokenAsset.token()->chainID() == asset.coinID()) {
                activeTokens.emplace_back(tokenAsset);
            }
        }

        chainSyncManager.reset(
            new T(chain, _transactionsCache, asset, _accountsCashe, _accountDataSource,
                std::move(web3Client), std::move(httpExplorerClient), activeTokens, nullptr));

        chainSyncManager->moveToThread(&_workerThread);
    }

    return chainSyncManager;
}

//==============================================================================
