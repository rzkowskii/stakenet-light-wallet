#include "AbstractEthGasProvider.hpp"

//==============================================================================

AbstractEthGasProvider::AbstractEthGasProvider(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

QString AbstractEthGasProvider::GasPrices::toString() const
{
    return QString("{ Slow: %1 GWEI, Average: %2 GWEI, Fast: %3 GWEI }")
        .arg((slow / eth::shannon).convert_to<int64_t>())
        .arg((standard / eth::shannon).convert_to<int64_t>())
        .arg((fast / eth::shannon).convert_to<int64_t>());
}

//==============================================================================
