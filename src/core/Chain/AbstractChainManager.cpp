#include "AbstractChainManager.hpp"
#include <Chain/Chain.hpp>
#include <Utils/Utils.hpp>

//==============================================================================

AbstractChainManager::AbstractChainManager(const WalletAssetsModel& assetsModel, QObject* parent)
    : QObject(parent)
    , _executionContext(new QObject(this))
    , _assetsModel(assetsModel)
{
}

//==============================================================================

AbstractChainManager::~AbstractChainManager() {}

//==============================================================================

Promise<std::shared_ptr<ChainView>> AbstractChainManager::getChainView(
    AssetID id, ChainViewUpdatePolicy policy) const
{
    return Promise<std::shared_ptr<ChainView>>([=](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            if (_chains.count(id) > 0) {
                std::shared_ptr<ChainView> result(
                    new ChainView(
                        _chains.at(id).get(), policy == ChainViewUpdatePolicy::CompressedEvents),
                    [](ChainView* obj) { obj->deleteLater(); });

                resolver(result);
            } else {
                reject(nullptr);
            }
        });
    });
}

//==============================================================================

AbstractMutableChainManager::AbstractMutableChainManager(
    const WalletAssetsModel& assetsModel, QObject* parent)
    : AbstractChainManager(assetsModel, parent)
{
}

//==============================================================================

bool AbstractMutableChainManager::hasChain(AssetID assetId) const
{
    return _chains.count(assetId) > 0;
}

//==============================================================================

Chain& AbstractMutableChainManager::chainById(AssetID assetId) const
{
    return *_chains.at(assetId);
}

//==============================================================================

std::vector<AssetID> AbstractMutableChainManager::chains() const
{
    std::vector<AssetID> chains;
    for (auto&& it : _chains) {
        chains.emplace_back(it.first);
    }

    return chains;
}

//==============================================================================
