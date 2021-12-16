#ifndef CONNEXTPAYMENTSPROXY_HPP
#define CONNEXTPAYMENTSPROXY_HPP

#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <Data/TransactionEntry.hpp>

#include <QObject>
#include <QPointer>

class AbstractTransactionsCache;
class AbstractConnextApi;
class WalletAssetsModel;

class ConnextPaymentsProxy : public QObject {
    Q_OBJECT
public:
    explicit ConnextPaymentsProxy(AssetID assetID, AbstractTransactionsCache* txCache,
                                    AbstractConnextApi* connextClient, const WalletAssetsModel& assetsModel, QObject* parent = nullptr);
    ~ConnextPaymentsProxy();

private slots:
    void onConnected();

private:
    void init();
    void refreshPayments();
    ConnextPaymentRef parsePayment(QVariantMap obj, QString identifier);

private:
    AssetID _assetID;
    QPointer<AbstractTransactionsCache> _txCache;
    QPointer<AbstractConnextApi> _connextClient;
    QPointer<QTimer> _connectionTimer;
    bool _hasConnection{ false };
    const WalletAssetsModel& _assetsModel;
};

#endif // CONNEXTPAYMENTSPROXY_HPP
