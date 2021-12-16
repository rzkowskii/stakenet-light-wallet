#ifndef LNPAYMENTSSYNCMANAGER_HPP
#define LNPAYMENTSSYNCMANAGER_HPP

#include <LndTools/Protos/LndTypes.pb.h>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

#include <QObject>
#include <QPointer>

class LndGrpcClient;
class AbstractTransactionsCache;

class LnPaymentsProxy : public QObject {
    Q_OBJECT
public:
    explicit LnPaymentsProxy(AssetID assetID, AbstractTransactionsCache* txCache,
        LndGrpcClient* client, QObject* parent = nullptr);

    Promise<QString> addInvoice(int64_t satoshisAmount);
    Promise<QString> makePayment(QString paymentRequest);

signals:

private slots:
    void onConnected();

private:
    void init();
    void subscribeSingleInvoice(std::string rHash, lndtypes::LightingInvoiceReason initialReason);

private:
    QPointer<LndGrpcClient> _client;
    QPointer<QTimer> _connectionTimer;
    QPointer<AbstractTransactionsCache> _txCache;
    AssetID _assetID;
    bool _hasConnection{ false };
};

#endif // LNPAYMENTSSYNCMANAGER_HPP
