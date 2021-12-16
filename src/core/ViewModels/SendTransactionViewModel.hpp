#ifndef SENDTRANSACTIONVIEWMODEL_HPP
#define SENDTRANSACTIONVIEWMODEL_HPP

#include <QObject>
#include <QPointer>
#include <array>

#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

class ApplicationViewModel;
class SendTransactionModel;

class SendTransactionViewModel : public QObject {
    Q_OBJECT

public:
    explicit SendTransactionViewModel(QObject* parent = nullptr);
    ~SendTransactionViewModel();

signals:
    void transactionCreated(qint64 moneyToSend, qint64 networkFee, QString recipientAddress);
    void transactionCreatingFailed(QString errorMessage);
    void transactionSendingFailed(QString error);
    void transactionSendingFinished(QString transactionID);
    void transactionResubmitted(QString txId);
    void transactionResubmitFailed(QString txId, QString errorMessage);

public slots:
    void initialize(ApplicationViewModel* applicationViewModel);
    void createSendTransaction(QVariantMap payload);
    void cancelSending();
    void confirmSending();
    void requestAddressDetails(AssetID assetID);
    bool validateAddress(QString input);
    void resubmit(QString txId);

private:
    void connectSignals();

private:
    QPointer<ApplicationViewModel> _applicationViewModel;
    QPointer<SendTransactionModel> _sendTransactionModel;
};

#endif // SENDTRANSACTIONVIEWMODEL_HPP
