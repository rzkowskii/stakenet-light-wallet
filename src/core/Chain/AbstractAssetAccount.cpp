#include "AbstractAssetAccount.hpp"

//==============================================================================

AbstractAssetAccount::AbstractAssetAccount(QString tokenAddress, QObject* parent)
    : QObject(parent)
    , _tokenAddress(tokenAddress)
{
}

//==============================================================================

QString AbstractAssetAccount::tokenAddress() const
{
    return _tokenAddress;
}

//==============================================================================

AbstractMutableAccount::~AbstractMutableAccount() {}

//==============================================================================

AssetsAccountsCache::AssetsAccountsCache(QObject* parent)
    : QObject(parent)
    , _executionContext(new QObject(this))
{
}

//==============================================================================

Promise<AbstractAssetAccount*> AssetsAccountsCache::cacheById(AssetID assetId)
{
    return Promise<AbstractAssetAccount*>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(
            _executionContext, [=] { resolve(&this->cacheByIdSync(assetId)); });
    });
}

//==============================================================================
