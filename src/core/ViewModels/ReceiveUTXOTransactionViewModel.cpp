#include "ReceiveUTXOTransactionViewModel.hpp"
#include <Models/WalletDataSource.hpp>

ReceiveUTXOTransactionViewModel::ReceiveUTXOTransactionViewModel(
    WalletDataSource& dataSource, AssetID assetId, bool isLightning, QObject* parent)
    : ReceiveTransactionViewModel(CoinAsset::Type::UTXO, isLightning, parent)
    , _walletDataSource(dataSource)
    , _assetId(assetId)

{
}

//==============================================================================

void ReceiveUTXOTransactionViewModel::requestAllKnownAddressesById()
{
    _walletDataSource.getAddressType(_assetId).then([this](Enums::AddressType type) {
        _walletDataSource.getReceivingAddress(_assetId, type).then([this](QString address) {
            _walletDataSource.getAllKnownAddressesById(_assetId).then(
                [this, address](AddressesList addressesById) {
                    QStringList addresses{ address };
                    std::move(std::begin(addressesById), std::end(addressesById),
                        std::back_inserter(addresses));
                    allKnownAddressByIdGenerated(addresses);
                });
        });
    });
}

//==============================================================================

void ReceiveUTXOTransactionViewModel::setAddressType(Enums::AddressType addressType)
{
    _walletDataSource.setAddressType(_assetId, addressType)
        .then([] {

        })
        .fail([](const std::exception& ex) {

        });
}

//==============================================================================

void ReceiveUTXOTransactionViewModel::requestAddressType()
{
    QPointer<ReceiveUTXOTransactionViewModel> self{ this };
    _walletDataSource.getAddressType(_assetId).then([self](Enums::AddressType addressType) {
        if (self) {
            bool isExist = addressType != Enums::AddressType::NONE;
            self->requestAddressTypeFinished(self->_assetId, isExist);
        }
    });
}

//==============================================================================
