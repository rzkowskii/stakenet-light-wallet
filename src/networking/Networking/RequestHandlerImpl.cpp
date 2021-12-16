#include "RequestHandlerImpl.hpp"
#include <Networking/NetworkConnectionState.hpp>
#include <Utils/Logging.hpp>

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkProxy>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QUrlQuery>
#include <functional>

#define NetworkNotAvailableMessage tr("Network is not available.")

//==============================================================================

QString GetOnionHost()
{
#ifdef Q_OS_ANDROID
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
#else
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation));
#endif
    auto innerPath = "Stakenet/stakenet-wallet/tor/onion";
    if (!dir.exists(innerPath)) {
        throw std::runtime_error("Failed to find onion dir");
    }
    dir.cd(innerPath);

    QFile file(dir.absoluteFilePath("hostname"));
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text) || !file.exists()) {
        throw std::runtime_error("Failed to get onion`s host");
    }

    QTextStream in(&file);
    QString host = in.readLine();
    return host;
}

//==============================================================================

RequestHandlerImpl::RequestHandlerImpl(QNetworkAccessManager* networkManager,
    NetworkConnectionState* connectionState, const Domains& domains, QObject* parent)
    : RequestHandler(parent)
    , _networkAccessManager(networkManager)
    , _connectionState(connectionState)
    , _domains(domains)
{
}

//==============================================================================

RequestHandlerImpl::~RequestHandlerImpl() {}

//==============================================================================

void RequestHandlerImpl::makeGetRequest(const QString& path, const QVariantMap& params,
    RequestHandlerImpl::NetworkErrorHandler errorHandler,
    RequestHandlerImpl::ResponseHandler responseHandler, int retryAttempts)
{
    setProxy();
    QNetworkRequest request(buildUrl(path, params));

    LogCDebug(Api) << request.url();

    RequestMaker requestMaker = [this, request]() { return _networkAccessManager->get(request); };

    processRequest(requestMaker, errorHandler, responseHandler, retryAttempts);
}

//==============================================================================

void RequestHandlerImpl::makePostRequest(const QString& path, const QJsonDocument& body,
    NetworkErrorHandler errorHandler, ResponseHandler responseHandler, int retryAttempts)
{
    setProxy();
    QNetworkRequest request(buildUrl(path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    LogCDebug(Api) << request.url();

    RequestMaker requestMaker
        = [this, request, body]() { return _networkAccessManager->post(request, body.toJson()); };

    processRequest(requestMaker, errorHandler, responseHandler, retryAttempts);
}

//==============================================================================

void RequestHandlerImpl::makePutRequest(const QString& path, const QVariantMap& params,
    RequestHandlerImpl::NetworkErrorHandler errorHandler,
    RequestHandlerImpl::ResponseHandler responseHandler, int retryAttempts)
{
    setProxy();
    QNetworkRequest request(buildUrl(path));
    QUrlQuery query;
    for (auto it = params.constBegin(); it != params.constEnd(); ++it) {
        query.addQueryItem(it.key(), it.value().toString());
    }
    QByteArray data = query.toString(QUrl::FullyEncoded).toUtf8();
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setHeader(QNetworkRequest::ContentLengthHeader, data.size());

    RequestMaker requestMaker
        = [this, request, data]() { return _networkAccessManager->put(request, data); };

    processRequest(requestMaker, errorHandler, responseHandler, retryAttempts);
}

//==============================================================================

void RequestHandlerImpl::makeDeleteRequest(const QString& path, const QJsonDocument& body,
    RequestHandler::NetworkErrorHandler errorHandler,
    RequestHandler::ResponseHandler responseHandler, int retryAttempts)
{
    Q_UNUSED(path)
    Q_UNUSED(body)
    Q_UNUSED(errorHandler)
    Q_UNUSED(responseHandler)
    Q_UNUSED(retryAttempts)
    throw std::runtime_error("Qt doesn't support DELETE requests");
}

//==============================================================================

void RequestHandlerImpl::processResponse(QNetworkReply* reply,
    RequestHandlerImpl::NetworkErrorHandler errorHandler,
    RequestHandlerImpl::ResponseHandler responseHandler)
{
    QByteArray response = reply->readAll();
    if (reply->error() == QNetworkReply::NoError) {
        //        if(_connectionState) {
        //            _connectionState->gossipConnectionState(NetworkConnectionState::State::Connected);
        //        }
        responseHandler(response);
    } else {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const auto& errorString = reply->errorString();
        LogCCritical(Api) << "HTTP error" << statusCode << errorString << Qt::endl << response;
        //        if (_connectionState && statusCode == 0) {
        //            _connectionState->gossipConnectionState(NetworkConnectionState::State::Disconnected);
        //        }

        if (!response.isEmpty()) {
            errorHandler(statusCode, response);
        } else {
            errorHandler(statusCode, errorString);
        }
    }
}

//==============================================================================

void RequestHandlerImpl::processRequest(RequestHandlerImpl::RequestMaker requestMaker,
    RequestHandlerImpl::NetworkErrorHandler errorHandler,
    RequestHandlerImpl::ResponseHandler responseHandler, int retryAttempts)
{
    auto reply = requestMaker();

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::OperationCanceledError && retryAttempts > 0) {
            LogWarning() << "Retrying request, attempts" << retryAttempts;
            processRequest(requestMaker, errorHandler, responseHandler, retryAttempts - 1);
        } else {
            processResponse(reply, errorHandler, responseHandler);
        }
    });
}

//==============================================================================

QUrl RequestHandlerImpl::buildUrl(const QString& path, const QVariantMap& params)
{
    QUrl url(QString("%1%2").arg(_useProxy ? _domains.tor : _domains.normal).arg(path));
    QUrlQuery query;
    for (auto it : params.toStdMap()) {
        query.addQueryItem(it.first, QUrl::toPercentEncoding(it.second.toString()));
    }
    url.setQuery(query);
    return url;
}

//==============================================================================

void RequestHandlerImpl::setProxy()
{
    _useProxy = QNetworkProxy::applicationProxy() != QNetworkProxy::NoProxy;
    if (_networkAccessManager->proxy() != QNetworkProxy::applicationProxy()) {
        _networkAccessManager->setProxy(QNetworkProxy::applicationProxy());
        if (_useProxy) {
            _networkAccessManager->connectToHost(QString("https://%1/").arg(GetOnionHost()), 21102);
        }
    }
}

//==============================================================================

QNetworkAccessManager* RequestHandlerImpl::networkAccessManager() const
{
    return _networkAccessManager;
}

//==============================================================================
