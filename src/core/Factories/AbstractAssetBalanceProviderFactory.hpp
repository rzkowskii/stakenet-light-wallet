#ifndef ABSTRACTASSETBALANCEPROVIDERFACTORY_HPP
#define ABSTRACTASSETBALANCEPROVIDERFACTORY_HPP

#include <Data/CoinAsset.hpp>

class AbstractAssetBalanceProvider;

struct AbstractAssetBalanceProviderFactory {
    virtual ~AbstractAssetBalanceProviderFactory();

    virtual std::unique_ptr<AbstractAssetBalanceProvider> createBalanceProvider(CoinAsset asset)
        = 0;
};

#endif // ABSTRACTASSETBALANCEPROVIDERFACTORY_HPP
