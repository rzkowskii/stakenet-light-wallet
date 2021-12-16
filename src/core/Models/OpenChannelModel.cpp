#include "OpenChannelModel.hpp"
#include <Data/WalletAssetsModel.hpp>

//==============================================================================

OpenChannelModel::OpenChannelModel(
    AssetID assetID, WalletAssetsModel* walletAssetsModel, QObject* parent)
    : QObject(parent)
    , _walletAssetsModel(walletAssetsModel)
    , _assetID(assetID)
{
}

//==============================================================================

AssetID OpenChannelModel::assetID() const
{
    return _assetID;
}

//==============================================================================
