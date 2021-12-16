#ifndef ASSETBALANCEPROVIDERFACTORY_HPP
#define ASSETBALANCEPROVIDERFACTORY_HPP

#include <Factories/AbstractAssetBalanceProviderFactory.hpp>
#include <QObject>

class AssetsTransactionsCache;
class AbstractChainManager;
class PaymentNodesManager;
class AssetsAccountsCache;

class AssetBalanceProviderFactory : public AbstractAssetBalanceProviderFactory {
public:
    explicit AssetBalanceProviderFactory(QPointer<AssetsTransactionsCache> assetsTxnsCache,
        QPointer<AssetsAccountsCache> accountsCache, QPointer<AbstractChainManager> chainManager,
        QPointer<PaymentNodesManager> nodeManager);
    // AbstractAssetBalanceProviderFactory interface
public:
    std::unique_ptr<AbstractAssetBalanceProvider> createBalanceProvider(CoinAsset asset) override;

private:
    QPointer<AssetsTransactionsCache> _assetsTxnsCache;
    QPointer<AssetsAccountsCache> _accountsCache;
    QPointer<AbstractChainManager> _chainManager;
    QPointer<PaymentNodesManager> _nodeManager;
};

#endif // ASSETBALANCEPROVIDERFACTORY_HPP
