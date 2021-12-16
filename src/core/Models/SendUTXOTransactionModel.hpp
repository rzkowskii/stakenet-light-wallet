#ifndef SENDUTXOTRANSACTIONMODEL_HPP
#define SENDUTXOTRANSACTIONMODEL_HPP

#include <Models/SendTransactionModel.hpp>
#include <QObject>
#include <Utils/Utils.hpp>

class WalletDataSource;
class AbstractNetworkingFactory;
class AbstractTransactionsCache;
class AbstractBlockExplorerHttpClient;

class SendUTXOTransactionModel : public SendTransactionModel {
    Q_OBJECT
public:
    explicit SendUTXOTransactionModel(AssetID assetID, WalletAssetsModel* walletAssetsModel,
        WalletDataSource* dataSource, AbstractNetworkingFactory* apiClientFactory,
        AbstractTransactionsCache* transactionsCache, QObject* parent = nullptr);

    // SendTransactionModel interface
public:
    bool validateAddress(QString address) override;
    void createSendTransaction(const SendTransactionParams& params) override;
    void confirmSending() override;
    void cancelSending() override;
    void resubmitTransaction(QString txId) override;

private slots:
    void onSendTransactionCreated(MutableTransactionRef transaction);

private:
    void requestAddressDetails();

private:
    QPointer<WalletDataSource> _walletDataSource;
    QPointer<AbstractTransactionsCache> _transactionsCache;
    qobject_delete_later_unique_ptr<AbstractBlockExplorerHttpClient> _apiClient;
    MutableTransactionRef _createdTx;
    std::vector<AddressEntry> _addressesEntry;
};

#endif // SENDUTXOTRANSACTIONMODEL_HPP
