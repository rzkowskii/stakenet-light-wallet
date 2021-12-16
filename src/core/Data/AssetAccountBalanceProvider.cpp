#include "AssetAccountBalanceProvider.hpp"

#include <Chain/AbstractAssetAccount.hpp>
#include <Models/ConnextDaemonInterface.hpp>

//==============================================================================

AssetAccountBalanceProvider::AssetAccountBalanceProvider(
    CoinAsset coinAsset, QPointer<AssetsAccountsCache> accountsCache, QPointer<ConnextDaemonInterface> connextInterface, QObject* parent)
    : AbstractAssetBalanceProvider(parent)
    , _asset(coinAsset)
    , _connextInterface(connextInterface)
{
    Q_ASSERT(accountsCache);
    init(accountsCache);
}

//==============================================================================

void AssetAccountBalanceProvider::update() {}

//==============================================================================

void AssetAccountBalanceProvider::init(QPointer<AssetsAccountsCache> accountsCache)
{
    auto assetID = _asset.coinID();
    connect(accountsCache, &AssetsAccountsCache::cacheAdded, this,
        [this, assetID, accountsCache](auto addedAssetID) {
            if (assetID != addedAssetID) {
                return;
            }

            accountsCache->cacheById(assetID).then([this](AbstractAssetAccount* accountCache) {
                connect(accountCache, &AbstractAssetAccount::balanceChanged, this,
                    &AssetAccountBalanceProvider::onBalanceChanged);

                accountCache->balance().then(
                    [this](Balance newBalance) { onBalanceChanged(newBalance); });
            });
        });

    connect(_connextInterface, &ConnextDaemonInterface::channelsBalanceChanged, this,
        [this](ConnextDaemonInterface::ConnextBalance newBalance) {
            AssetBalance result;
            result.nodeBalance = static_cast<qint64>(newBalance.localBalance);
            result.availableNodeBalance = static_cast<qint64>(newBalance.localBalance);
            result.activeNodeBalance = static_cast<qint64>(newBalance.localBalance);
            result.balance = balance().balance;
            result.confirmedBalance = balance().confirmedBalance;
            this->setBalance(result);
        });
}

//==============================================================================

void AssetAccountBalanceProvider::onBalanceChanged(Balance newBalance)
{
    AssetBalance result;
    result.balance = newBalance;
    result.confirmedBalance = newBalance;
    result.nodeBalance = balance().nodeBalance;
    result.availableNodeBalance = balance().availableNodeBalance;
    result.activeNodeBalance = balance().activeNodeBalance;
    this->setBalance(result);
}

//==============================================================================
