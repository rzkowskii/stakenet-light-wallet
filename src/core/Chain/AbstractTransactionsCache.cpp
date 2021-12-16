#include "AbstractTransactionsCache.hpp"
#include <Utils/Utils.hpp>

//==============================================================================

AbstractTransactionsCache::AbstractTransactionsCache(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

AssetsTransactionsCache::AssetsTransactionsCache(QObject* parent)
    : QObject(parent)
    , _executionContext(new QObject(this))
{
}

//==============================================================================

Promise<AbstractTransactionsCache*> AssetsTransactionsCache::cacheById(AssetID assetId)
{
    return Promise<AbstractTransactionsCache*>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(
            _executionContext, [=] { resolve(&this->cacheByIdSync(assetId)); });
    });
}

//==============================================================================
