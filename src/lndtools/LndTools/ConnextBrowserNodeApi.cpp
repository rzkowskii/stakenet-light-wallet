#include "ConnextBrowserNodeApi.hpp"
#include <QDateTime>
#include <QJSValue>
#include <Utils/Logging.hpp>

//==============================================================================

ConnextBrowserNodeApi::ConnextBrowserNodeApi(
    SendTransactionDelegate sendTxDelegate, QObject* parent)
    : AbstractConnextApi(parent)
    , _transport{ new ConnextBrowserNodeApiTransport{ this } }
    , _sendTxDelegate(sendTxDelegate)
{
}

//==============================================================================

Promise<QString> ConnextBrowserNodeApi::getPublicIdentifier()
{
    return Promise<QString>::resolve(_publicIdentifier);
}

//==============================================================================

Promise<std::vector<QString>> ConnextBrowserNodeApi::getChannelsAddresses(QString publicIdentifier)
{
    Q_UNUSED(publicIdentifier)
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(&ConnextBrowserNodeApiTransport::getStateChannels, {}, resolve, reject);
    })
        .then([this](QVariant response) {
            auto addressesList = response.toList();

            std::vector<QString> channelAddresses;
            std::transform(std::begin(addressesList), std::end(addressesList),
                std::back_inserter(channelAddresses),
                [](const auto address) { return address.toString(); });

            return channelAddresses;
        });
}

//==============================================================================

Promise<QVariantMap> ConnextBrowserNodeApi::getChannel(
    QString publicIdentifier, QString channelAddress)
{
    Q_UNUSED(publicIdentifier)
    QVariantMap payload;

    payload["channelAddress"] = channelAddress;

    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(
            &ConnextBrowserNodeApiTransport::getStateChannel, payload, resolve, reject);
    })
        .then([](QVariant response) { return response.toMap(); })
        .fail([channelAddress]() {
            qDebug() << "Get channel failed" << channelAddress;
            return Promise<QVariantMap>::reject(
                std::runtime_error(QString("Get channel failed! Channel address %1")
                                       .arg(channelAddress)
                                       .toStdString()));
        });
}

//==============================================================================

Promise<QByteArray> ConnextBrowserNodeApi::transferResolve(QVariantMap payload)
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(
            &ConnextBrowserNodeApiTransport::resolveTransfer, payload, resolve, reject);
    })
        .then([](QVariant response) { return response.toByteArray(); })
        .fail([]() {
            qDebug() << "transferResolve failed";
            return Promise<QByteArray>::reject(
                std::runtime_error("Resolve transfer channel failed!"));
        });
}

//==============================================================================

Promise<QVector<QVariantMap>> ConnextBrowserNodeApi::getChannelsList(QString publicIdentifier)
{
    Q_UNUSED(publicIdentifier)
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(&ConnextBrowserNodeApiTransport::getStateChannels, {}, resolve, reject);
    })
        .then([this](QVariant response) {
            auto addressesList = response.toList();
//            qDebug() << "State channels:" << addressesList;

            QVector<QString> channelAddresses;
            std::transform(std::begin(addressesList), std::end(addressesList),
                std::back_inserter(channelAddresses),
                [](const auto address) { return address.toString(); });

            return QtPromise::map(channelAddresses,
                [this](const QString& address, ...) {
                    return getChannel(QString(), address)
                        .then([](QVariantMap channel) { return channel; })
                        .tapFail([address] { LogCCInfo(Connext) << "Get channel failed!"; });
                })
                .then([](const QVector<QVariantMap>& channels) { return channels; });
        });
}

//==============================================================================

Promise<QVector<QVariantMap>> ConnextBrowserNodeApi::getTransfers(QVariantMap payload)
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(&ConnextBrowserNodeApiTransport::getTransfers, payload, resolve, reject);
    })
        .then([](QVariant response) {
//            qDebug() << "getTransfers response" << response;
            QVector<QVariantMap> result;
            auto payments = response.toList();
            for (auto payment : payments) {
                result.push_back(payment.toMap());
            }
            return result;
        })
        .fail([](ConnextApiException error) {
            qDebug() << "Get transfers failed!";
            qDebug() << "Message: " << error.msg;
            qDebug() << "validationError" << error.validationError;
            return Promise<QVector<QVariantMap>>::reject(error);
        });
}

//==============================================================================

bool ConnextBrowserNodeApi::isActive()
{
    return !_publicIdentifier.isEmpty();
}

//==============================================================================

Promise<QString> ConnextBrowserNodeApi::transferCreate(QVariantMap payload)
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(
            &ConnextBrowserNodeApiTransport::conditionalTransfer, payload, resolve, reject);
    })
        .then([](QVariant response) { return response.toMap().value("transferId").toString(); })
        .fail([]() {
            qDebug() << "transferCreate failed";
            return Promise<QString>::reject(std::runtime_error("Transfer create failed!"));
        });
}

//==============================================================================

Promise<QString> ConnextBrowserNodeApi::setupChannel(QVariantMap payload)
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(&ConnextBrowserNodeApiTransport::setup, payload, resolve, reject);
    })
        .then([](QVariant response) {
            qDebug() << "Setup channel finised!" << response;
            LogCCInfo(Connext) << "Setup channel finised!" << response;
            return response.toMap().value("channelAddress").toString();
        })
        .fail([](ConnextApiException error) {
            qDebug() << "Setup channel failed!";
            qDebug() << "Message: " << error.msg;
            qDebug() << "validationError" << error.validationError;
            return Promise<QString>::reject(error);
        });
}

//==============================================================================

Promise<void> ConnextBrowserNodeApi::sendDeposit(DepositTxParams payload)
{
    return _sendTxDelegate(payload)
        .fail([](const std::exception& ex) {
            return Promise<void>::reject(std::current_exception());
        })
        .fail([]() { return Promise<void>::reject(std::current_exception()); });
}

//==============================================================================

Promise<QString> ConnextBrowserNodeApi::reconcile(QVariantMap payload)
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(
            &ConnextBrowserNodeApiTransport::reconcileDeposit, payload, resolve, reject);
    })
        .then([](QVariant response) {
            qDebug() << "Reconcile channel finised!" << response;
            LogCCInfo(Connext) << "Reconcile channel finised!" << response;
            return response.toMap().value("channelAddress").toString();
        })
        .fail([](ConnextApiException error) {
            qDebug() << "Reconcile failed!";
            qDebug() << "Message: " << error.msg;
            qDebug() << "validationError" << error.validationError;
            return Promise<QString>::reject(error);
        });
}

//==============================================================================

Promise<void> ConnextBrowserNodeApi::withdraw(QVariantMap payload)
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(
            &ConnextBrowserNodeApiTransport::withdrawDeposit, payload, resolve, reject);
    })
        .then([](QVariant response) {
            qDebug() << "Setup channel finised!" << response;
            LogCCInfo(Connext) << "Withdraw channel finised!" << response;
        })
        .fail([](ConnextApiException error) {
            qDebug() << "Withdraw failed!";
            qDebug() << "Message: " << error.msg;
            qDebug() << "validationError" << error.validationError;
            return Promise<void>::reject(error);
        });
}

//==============================================================================

Promise<void> ConnextBrowserNodeApi::initNode(QString secret)
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QVariantMap payload;
        //        payload["privKey"]
        //            = QString{
        //            "0x2df591b7552895e90b6cd24cfde781c59e1afe38fc529399421cbd977db602fa" };
        payload["privKey"] = secret;
        payload["authUrl"] = QString{ "https://messaging.connext.network" };
        payload["natsUrl"] = QString{ "wss://websocket.connext.provide.network" };
        // payload["messagingUrl"] = QString{ "https://messaging.connext.network" };
        payload["chainProviders"]
            = QVariantMap{ { "4", "https://rinkeby.infura.io/v3/bd8cb95e87bc4e3fbe58f8db6609e995" },
                  { "1", "https://mainnet.infura.io/v3/bd8cb95e87bc4e3fbe58f8db6609e995" } };

        _transport->invoke(&ConnextBrowserNodeApiTransport::initialize, payload, resolve, reject);
    })
        .then([this](QVariant payload) {
            LogCCInfo(Connext) << "Connext browser node succesfully initialized" << payload;
            _publicIdentifier = payload.toString();
            if (!_publicIdentifier.isEmpty()) {
                emit connected();
            }
        });
}

//==============================================================================

Promise<QString> ConnextBrowserNodeApi::restoreState(QVariantMap payload)
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        _transport->invoke(&ConnextBrowserNodeApiTransport::restoreState, payload, resolve, reject);
    })
        .then([](QVariant response) {
            qDebug() << "Restore state finised!" << response;
            LogCCInfo(Connext) << "Restore state finised!" << response;
            return response.toMap().value("channelAddress").toString();
        })
        .fail([](ConnextApiException error) {
            qDebug() << "Restore state failed!";
            qDebug() << "Message: " << error.msg;
            qDebug() << "validationError" << error.validationError;
            return Promise<QString>::reject(error);
        });
}

//==============================================================================

ConnextBrowserNodeApiTransport* ConnextBrowserNodeApi::transport() const
{
    return _transport;
}

//==============================================================================

ConnextBrowserNodeApiTransport::ConnextBrowserNodeApiTransport(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

qint32 ConnextBrowserNodeApiTransport::generateSeq() const
{
    return _handlers.empty() ? 0 : (_handlers.rbegin()->first + 1);
}

//==============================================================================

void ConnextBrowserNodeApiTransport::dispatch(qint32 seq, QVariant payload)
{
    if (_handlers.count(seq) == 0) {
        LogCCritical(Connext) << "Could not find handler for seq:" << seq;
        return;
    }

    if (payload.canConvert<QJSValue>()) {
        payload = payload.value<QJSValue>().toVariant();
    }

    auto it = _handlers.find(seq);
    std::get<0>(it->second)(payload);
    _handlers.erase(it);
}

//==============================================================================

void ConnextBrowserNodeApiTransport::dispatchError(qint32 seq, QVariant error)
{
    if (_handlers.count(seq) == 0) {
        LogCCritical(Connext) << "Could not find handler for seq:" << seq;
        return;
    }

    if (error.canConvert<QJSValue>()) {
        error = error.value<QJSValue>().toVariant();
    }

    // sample error after: error.value<QJSValue>().toVariant()
    // QVariant(QVariantMap, QMap(("code", QVariant(QString, "NATS_PROTOCOL_ERR"))("name",
    // QVariant(QString, "NatsError"))))
    qDebug() << "Dispatching error:" << error;

    auto it = _handlers.find(seq);
    std::get<1>(it->second)(ConnextApiException(error.toMap()));
    _handlers.erase(it);
}

//==============================================================================
