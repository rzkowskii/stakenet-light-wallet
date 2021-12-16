#include "ConnextHttpClient.hpp"
#include <Networking/NetworkingUtils.hpp>
#include <Utils/Logging.hpp>

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>
#include <QUrlQuery>

//==============================================================================

ConnextHttpClient::ConnextHttpClient(
    std::unique_ptr<RequestHandlerImpl>&& requestHandler, QObject* parent)
    : AbstractConnextApi(parent)

{
    requestHandler->setParent(this);
    _requestHandler = requestHandler.release();
}

//==============================================================================

Promise<QString> ConnextHttpClient::getPublicIdentifier()
{
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const auto& response) {
                LogCDebug(Swaps) << "Got address response:" << response;
                auto address = QJsonDocument::fromJson(response)
                                   .array()[0]
                                   .toObject()
                                   .value("publicIdentifier")
                                   .toString();
                resolve(address);
            };

            const QString path = QString("/config");
            _requestHandler->makeGetRequest(path, {}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<std::vector<QString>> ConnextHttpClient::getChannelsAddresses(QString publicIdentifier)
{
    return Promise<std::vector<QString>>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const auto& response) {
                std::vector<QString> channels;
                for (auto obj : QJsonDocument::fromJson(response).array()) {
                    channels.emplace_back(obj.toString());
                }
                resolve(channels);
            };

            const QString path = QString("/%1/channels").arg(publicIdentifier);
            _requestHandler->makeGetRequest(path, {}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<QVariantMap> ConnextHttpClient::getChannel(QString publicIdentifier, QString channelAddress)
{
    return Promise<QVariantMap>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const auto& response) {
                resolve(QJsonDocument::fromJson(response).object().toVariantMap());
            };

            const QString path
                = QString("/%1/channels/%2").arg(publicIdentifier).arg(channelAddress);
            _requestHandler->makeGetRequest(path, {}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<QByteArray> ConnextHttpClient::transferResolve(QVariantMap payload)
{
    return Promise<QByteArray>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const auto& response) {
                LogCDebug(Swaps) << "Got post transfer response:" << response;
                resolve(response);
            };

            const QString path = QString("/transfers/resolve");
            _requestHandler->makePostRequest(path,
                QJsonDocument(QJsonObject::fromVariantMap(payload)), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<QString> ConnextHttpClient::transferCreate(QVariantMap payload)
{
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const auto& response) {
                LogCDebug(Swaps) << "Got post transfer response:" << response;
                resolve(QJsonDocument::fromJson(response).object().value("transferId").toString());
            };

            const QString path = QString("/transfers/create");
            _requestHandler->makePostRequest(path,
                QJsonDocument(QJsonObject::fromVariantMap(payload)), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<QString> ConnextHttpClient::setupChannel(QVariantMap payload)
{
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode,
                    QJsonDocument::fromJson(errorResponse.toUtf8())
                        .object()
                        .value("message")
                        .toString() });
            };

            auto responseHandler = [=](const auto& response) {
                LogCDebug(Swaps) << "Got post setup response:" << response;
                auto channelAddress
                    = QJsonDocument::fromJson(response).object().value("channelAddress").toString();
                resolve(channelAddress);
            };

            const QString path = QString("/setup");
            _requestHandler->makePostRequest(path,
                QJsonDocument(QJsonObject::fromVariantMap(payload)), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<QVector<QVariantMap>> ConnextHttpClient::getChannelsList(QString publicIdentifier)
{
    return getChannelsAddresses(publicIdentifier)
        .then([this, publicIdentifier](std::vector<QString> channelsAddresses) {
            return QtPromise::map(
                channelsAddresses, [publicIdentifier, this](QString channelAddress, ...) {
                    return getChannel(publicIdentifier, channelAddress);
                });
        });
}

//==============================================================================

Promise<void> ConnextHttpClient::sendDeposit(DepositTxParams payload)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode,
                    QJsonDocument::fromJson(errorResponse.toUtf8())
                        .object()
                        .value("message")
                        .toString() });
            };

            auto responseHandler = [=](const auto& response) {
                LogCDebug(Swaps) << "Got post deposit response:" << response;
                resolve();
            };

            QVariantMap sendDepositPayload;

            sendDepositPayload["channelAddress"] = payload.channelAddress;
            sendDepositPayload["amount"] = payload.amount;
            sendDepositPayload["assetId"] = payload.tokenAddress;
            sendDepositPayload["chainId"] = QVariant::fromValue(payload.chainId);
            sendDepositPayload["publicIdentifier"] = payload.publicIdentifier;

            const QString path = QString("/send-deposit-tx");
            _requestHandler->makePostRequest(path,
                QJsonDocument(QJsonObject::fromVariantMap(sendDepositPayload)), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<QString> ConnextHttpClient::reconcile(QVariantMap payload)
{
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode,
                    QJsonDocument::fromJson(errorResponse.toUtf8())
                        .object()
                        .value("message")
                        .toString() });
            };

            auto responseHandler = [=](const auto& response) {
                LogCDebug(Swaps) << "Got post deposit response:" << response;
                auto channelAddress
                    = QJsonDocument::fromJson(response).object().value("channelAddress").toString();
                resolve(channelAddress);
            };

            const QString path = QString("/deposit");
            _requestHandler->makePostRequest(path,
                QJsonDocument(QJsonObject::fromVariantMap(payload)), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<void> ConnextHttpClient::withdraw(QVariantMap payload)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode,
                    QJsonDocument::fromJson(errorResponse.toUtf8())
                        .object()
                        .value("message")
                        .toString() });
            };

            auto responseHandler = [=](const auto& response) {
                LogCDebug(Swaps) << "Got withdraw response:" << response;
                resolve();
            };

            const QString path = QString("/withdraw");
            _requestHandler->makePostRequest(path,
                QJsonDocument(QJsonObject::fromVariantMap(payload)), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<void> ConnextHttpClient::initNode(QString mnemonic)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const auto& response) {
                LogCDebug(Swaps) << "Got initNode response:" << response;
                resolve();
            };

            QVariantMap payload;
            payload["index"] = 0;
            payload["mnemonic"] = mnemonic;

            const QString path = QString("/node");
            _requestHandler->makePostRequest(path,
                QJsonDocument(QJsonObject::fromVariantMap(payload)), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

Promise<QString> ConnextHttpClient::restoreState(QVariantMap payload)
{
    return Promise<QString>::reject(std::runtime_error("method not implemented"));
}

//==============================================================================

Promise<QVector<QVariantMap>> ConnextHttpClient::getTransfers(QVariantMap payload)
{
    return Promise<QVector<QVariantMap>>::reject(std::runtime_error("method not implemented"));
}

//==============================================================================

bool ConnextHttpClient::isActive()
{
    return false;
}

//==============================================================================
