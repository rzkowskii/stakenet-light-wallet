#ifndef SENDTRANSACTIONMODEL_HPP
#define SENDTRANSACTIONMODEL_HPP

#include <QObject>

#include <Data/SendTransactionParams.hpp>
#include <Data/TransactionEntry.hpp>
#include <Tools/Common.hpp>

class SendTransactionModel : public QObject {
    Q_OBJECT
public:
    explicit SendTransactionModel(
        AssetID assetID, WalletAssetsModel* walletAssetsModel, QObject* parent = nullptr);

    AssetID assetID() const;
    virtual bool validateAddress(QString address) = 0;
    virtual void createSendTransaction(const SendTransactionParams& params) = 0;
    virtual void confirmSending() = 0;
    virtual void cancelSending() = 0;
    virtual void resubmitTransaction(QString txId) = 0;

signals:
    void transactionCreated(qint64 moneyToSend, qint64 networkFee, QString recipientAddress);
    void transactionCreatingFailed(QString errorMessage);
    void transactionSendingFailed(QString error);
    void transactionSendingFinished(QString transactionID);
    void transactionResubmitted(QString txId);
    void transactionResubmitFailed(QString txId, QString errorMessage);

protected:
    QPointer<WalletAssetsModel> _walletAssetsModel;

private:
    AssetID _assetID;
};

#endif // SENDTRANSACTIONMODEL_HPP
