#ifndef SENDACCOUNTTRANSACTIONMODEL_HPP
#define SENDACCOUNTTRANSACTIONMODEL_HPP

#include <EthCore/Encodings.hpp>
#include <Models/SendTransactionModel.hpp>
#include <Utils/Utils.hpp>
#include <Tools/Common.hpp>

class AccountDataSource;
class AbstractNetworkingFactory;
class AbstractTransactionsCache;
class AbstractWeb3Client;
class AbstractEthGasProvider;
class WalletAssetsModel;

class SendAccountTransactionModel : public SendTransactionModel {
    Q_OBJECT
public:
    explicit SendAccountTransactionModel(AssetID assetID, WalletAssetsModel* walletAssetsModel,
        AccountDataSource* dataSource, qobject_delete_later_unique_ptr<AbstractWeb3Client> web3,
        AbstractTransactionsCache* transactionsCache, QObject* parent = nullptr);

    // SendTransactionModel interface
public:
    bool validateAddress(QString address) override;
    void createSendTransaction(const SendTransactionParams& params) override;
    void confirmSending() override;
    void cancelSending() override;
    void resubmitTransaction(QString txId) override;

    Promise<eth::SignedTransaction> createSignedSendTransaction(const SendTransactionParams& params, QString fromAddress) const;
    Promise<QString> sendTransaction(QString fromAddress, eth::SignedTransaction transaction);

private slots:
    void onSendTransactionCreated(std::optional<eth::AccountSendTxParams> params, QString address);
    void onSendTransactionFailed(QString error);

private:
    Promise<int64_t> calculateNonce(QString address) const;
    Promise<eth::u256> selectGasPrice(Enums::GasType type) const;

private:
    AccountDataSource* _dataSource;
    QPointer<AbstractTransactionsCache> _transactionsCache;
    qobject_delete_later_unique_ptr<AbstractWeb3Client> _web3;
    std::tuple<std::optional<eth::AccountSendTxParams>, QString> _createdTxParams;
    QPointer<AbstractEthGasProvider> _fallbackGasProvider;
    QPointer<AbstractEthGasProvider> _gasProvider;
    QPointer<WalletAssetsModel> _assetModel;
};

#endif // SENDACCOUNTTRANSACTIONMODEL_HPP
