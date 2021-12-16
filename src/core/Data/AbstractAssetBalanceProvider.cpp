#include "AbstractAssetBalanceProvider.hpp"

//==============================================================================

AbstractAssetBalanceProvider::AbstractAssetBalanceProvider(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

AssetBalance AbstractAssetBalanceProvider::balance() const
{
    return _balance;
}

//==============================================================================

void AbstractAssetBalanceProvider::setBalance(AssetBalance newBalance)
{
    if (_balance != newBalance) {
        _balance = newBalance;
        balanceChanged(newBalance);
    }
}

//==============================================================================

bool AssetBalance::operator==(const AssetBalance &rhs) const
{
    return balance == rhs.balance && confirmedBalance == rhs.confirmedBalance
        && nodeBalance == rhs.nodeBalance && availableNodeBalance == rhs.availableNodeBalance
            && activeNodeBalance == rhs.activeNodeBalance;
}

//==============================================================================

bool AssetBalance::operator!=(const AssetBalance &rhs) const
{
    return !((*this) == rhs);
}

//==============================================================================

Balance AssetBalance::total() const
{
    return balance + nodeBalance;
}

//==============================================================================
