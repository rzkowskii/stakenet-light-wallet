#ifndef LIGHTNINGSENDTRANSACTIONVIEWMODEL_HPP
#define LIGHTNINGSENDTRANSACTIONVIEWMODEL_HPP

#include <Models/LnDaemonInterface.hpp>
#include <Tools/Common.hpp>

#include <QObject>
#include <QPointer>
#include <QVariant>
#include <QVariantMap>

class ApplicationViewModel;
class LnPaymentsProxy;

class LightningSendTransactionViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(LightningPayRequest payRequest READ payRequest NOTIFY payRequestChanged)
public:
    explicit LightningSendTransactionViewModel(QObject* parent = nullptr);

    LightningPayRequest payRequest() const;

signals:
    void payRequestChanged();
    void payRequestError(QString payReqErr);
    void payRequestResponseReceived(QString destination, QString num_satoshis);

    void transactionSendingFailed(QString error);
    void transactionSendingFinished(QString transactionID);

    void payRequestReady(QString payReq);

public slots:
    void initialize(AssetID assetID, ApplicationViewModel* applicationViewModel);
    void addInvoice(double amoutInSatoshi);
    void decodePayRequest(QString payReq);
    void makePayment();
    void cancelPayment();

private:
    void setPayRequest(LightningPayRequest payReq);

private:
    QPointer<LnPaymentsProxy> _lnPayments;
    QPointer<PaymentNodeInterface> _daemonInterface;
    LightningPayRequest _payRequest;
    QString _encodedPayRequest;
};

#endif // LIGHTNINGSENDTRANSACTIONVIEWMODEL_HPP
