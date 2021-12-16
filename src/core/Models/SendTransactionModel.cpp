#include "SendTransactionModel.hpp"
#include <Data/WalletAssetsModel.hpp>

//==============================================================================

SendTransactionModel::SendTransactionModel(
    AssetID assetID, WalletAssetsModel* walletAssetsModel, QObject* parent)
    : QObject(parent)
    , _walletAssetsModel(walletAssetsModel)
    , _assetID(assetID)
{
}

//==============================================================================

AssetID SendTransactionModel::assetID() const
{
    return _assetID;
}

//==============================================================================
