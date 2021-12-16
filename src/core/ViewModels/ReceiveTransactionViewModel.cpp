#include "ReceiveTransactionViewModel.hpp"

ReceiveTransactionViewModel::ReceiveTransactionViewModel(CoinAsset::Type assetType, bool isLightning, QObject* parent)
    : QObject(parent)
    , _assetType(assetType)
    , _isLightning(isLightning)
{
}

//==============================================================================

ReceiveTransactionViewModel::~ReceiveTransactionViewModel() {}

//==============================================================================

CoinAsset::Type ReceiveTransactionViewModel::assetType() const
{
    return _assetType;
}

//==============================================================================

bool ReceiveTransactionViewModel::isLightning() const
{
    return _isLightning;
}

//==============================================================================

bool ReceiveTransactionViewModel::isUTXOType() const
{
    return _assetType ==  CoinAsset::Type::UTXO;
}

//==============================================================================



