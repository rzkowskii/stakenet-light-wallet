#ifndef RECEIVEACCOUNTTRANSACTIONVIEWMODEL_HPP
#define RECEIVEACCOUNTTRANSACTIONVIEWMODEL_HPP

#include <ViewModels/ReceiveTransactionViewModel.hpp>

class AccountDataSource;

class ReceiveAccountTransactionViewModel : public ReceiveTransactionViewModel {
    Q_OBJECT
public:
    ReceiveAccountTransactionViewModel(AccountDataSource& dataSource, AssetID assetId,
        bool isLightning, QObject* parent = nullptr);

public slots:
    void requestAllKnownAddressesById() override;

private:
    AccountDataSource& _accountDataSource;
    AssetID _assetId;
};

#endif // RECEIVEACCOUNTTRANSACTIONVIEWMODEL_HPP
