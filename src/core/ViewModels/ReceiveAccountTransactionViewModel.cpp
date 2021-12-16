#include "ReceiveAccountTransactionViewModel.hpp"
#include <Models/WalletDataSource.hpp>

ReceiveAccountTransactionViewModel::ReceiveAccountTransactionViewModel(
    AccountDataSource& dataSource, AssetID assetId, bool isLightning, QObject* parent)
    : ReceiveTransactionViewModel(CoinAsset::Type::Account, isLightning, parent)
    , _accountDataSource(dataSource)
    , _assetId(assetId)
{
}

//==============================================================================

void ReceiveAccountTransactionViewModel::requestAllKnownAddressesById()
{
    _accountDataSource.getAccountAddress(_assetId).then(
        [this](QString address) { allKnownAddressByIdGenerated({ address }); });
}

//==============================================================================
