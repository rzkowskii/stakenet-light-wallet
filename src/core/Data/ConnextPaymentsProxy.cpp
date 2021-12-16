#include "ConnextPaymentsProxy.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <LndTools/AbstractConnextApi.hpp>

#include <QJsonDocument>
#include <QJsonObject>

//==============================================================================

ConnextPaymentRef ConnextPaymentsProxy::parsePayment(QVariantMap obj, QString identifier)
{
    QJsonObject json = QJsonObject::fromVariantMap(obj);
    auto meta = json.value("meta").toObject();
    auto quote = meta.value("quote").toObject();
    auto contract = json.value("assetId").toString();

    auto coin = _assetsModel.assetByContract(contract);

    auto assetId = coin.coinID();
    auto decimals = coin.token() ? coin.token()->decimals() : 18;
    auto transferId = obj.value("transferId").toString();
    auto channelAddress = obj.value("channelAddress").toString();

    auto type = json.value("initiatorIdentifier").toString() == identifier
        ? chain::ConnextPayment::ConnextPaymentType::ConnextPayment_ConnextPaymentType_SEND
        : chain::ConnextPayment::ConnextPaymentType::ConnextPayment_ConnextPaymentType_RECEIVE;
    auto value
        = eth::u256{ ConvertFromDecimalWeiToHex(quote.value("amount").toString().toDouble()) };
    auto timestamp
        = QDateTime::fromMSecsSinceEpoch(static_cast<int64_t>(meta.value("createdAt").toDouble()));

    TxMemo txMemo;
    txMemo.emplace("decimals", QString::number(decimals).toStdString());

    return std::make_shared<ConnextPayment>(
        assetId, transferId, value, type, channelAddress, timestamp, txMemo);
}

//==============================================================================

ConnextPaymentsProxy::ConnextPaymentsProxy(AssetID assetID, AbstractTransactionsCache* txCache,
    AbstractConnextApi* connextClient, const WalletAssetsModel& assetsModel, QObject* parent)
    : QObject(parent)
    , _assetID(assetID)
    , _txCache(txCache)
    , _connextClient(connextClient)
    , _assetsModel(assetsModel)
{
    init();
}

//==============================================================================

ConnextPaymentsProxy::~ConnextPaymentsProxy()
{
    _connectionTimer->stop();
}

//==============================================================================

void ConnextPaymentsProxy::onConnected()
{
    if (!_connextClient->isActive()) {
        return;
    }

    refreshPayments();
}

//==============================================================================

void ConnextPaymentsProxy::init()
{
    connect(
        _connextClient, &AbstractConnextApi::connected, this, &ConnextPaymentsProxy::onConnected);

    _connectionTimer = new QTimer(this);
    _connectionTimer->setInterval(5000);
    connect(_connectionTimer, &QTimer::timeout, this, &ConnextPaymentsProxy::onConnected);
    _connectionTimer->start(_connectionTimer->interval());
}

//==============================================================================

void ConnextPaymentsProxy::refreshPayments()
{
    QPointer<ConnextPaymentsProxy> self{ this };

    self->_connextClient->getPublicIdentifier().then([self](QString identifier) {
        self->_txCache->connextPaymentsList().then([self, identifier](ConnextPaymentList paymentList) {
        QVariantMap payload;
        payload["startDate"] = !paymentList.empty() ? paymentList.back()->transactionDate().toMSecsSinceEpoch() : 0;
        payload["endDate"] = QDateTime::currentMSecsSinceEpoch();

        self->_connextClient->getTransfers(payload)
            .then([self, identifier](QVector<QVariantMap> payments) {
                for (auto payment : payments) {
                    auto parsedPayment = self->parsePayment(payment, identifier);
                    if(parsedPayment->assetID() == self->_assetID){
                        self->_txCache->addTransactions({parsedPayment});
                    }
                }
            })
            .fail([](ConnextApiException error) {
                qDebug() << "Get transfers failed!";
                qDebug() << "Message: " << error.msg;
                qDebug() << "validationError" << error.validationError;
            });
    });
   });
}

//==============================================================================
