#ifndef RECEIVEUTXOTRANSACTIONVIEWMODEL_HPP
#define RECEIVEUTXOTRANSACTIONVIEWMODEL_HPP

#include <ViewModels/ReceiveTransactionViewModel.hpp>

#include <Data/CoinAsset.hpp>
#include <QObject>

class WalletDataSource;
class ReceiveTransactionViewModel;

class ReceiveUTXOTransactionViewModel : public ReceiveTransactionViewModel {
    Q_OBJECT
public:
    ReceiveUTXOTransactionViewModel(
        WalletDataSource& dataSource, AssetID assetId, bool isLightning, QObject* parent = nullptr);

public slots:
    void requestAllKnownAddressesById() override;
    void setAddressType(Enums::AddressType addressType);
    void requestAddressType();

signals:
    void requestAddressTypeFinished(AssetID assetID, bool isExist);

private:
    WalletDataSource& _walletDataSource;
    AssetID _assetId;
};

#endif // RECEIVEUTXOTRANSACTIONVIEWMODEL_HPP
