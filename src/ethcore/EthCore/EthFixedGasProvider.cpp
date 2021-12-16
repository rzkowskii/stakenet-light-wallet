#include "EthFixedGasProvider.hpp"

//==============================================================================

EthFixedGasProvider::EthFixedGasProvider(GasPrices prices, QObject* parent)
    : AbstractEthGasProvider(parent)
    , _prices(prices)
{
}

//==============================================================================

Promise<AbstractEthGasProvider::GasPrices> EthFixedGasProvider::fetchGasPrice()
{
    return QtPromise::resolve(_prices);
}

//==============================================================================
