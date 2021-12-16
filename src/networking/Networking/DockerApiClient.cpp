#include "DockerApiClient.hpp"
#include "RequestHandlerImpl.hpp"
#include <Networking/NetworkingUtils.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrlQuery>

//==============================================================================

QUrl BuildUrl(const QString& path, const QVariantMap& params)
{
    QUrl url(path);
    QUrlQuery query;
    for (auto it : params.toStdMap()) {
        query.addQueryItem(it.first, QUrl::toPercentEncoding(it.second.toString()));
    }
    url.setQuery(query);
    return url;
}

//==============================================================================

DockerApiClient::DockerApiClient(std::unique_ptr<RequestHandler>&& requestHandler, QObject* parent)
    : QObject(parent)
    , _requestHandler(requestHandler.release())
{
    _requestHandler->setParent(this);
}

//==============================================================================

auto DockerApiClient::createImage(QString name, QString tag) -> Promise<void>
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler
                = [=](int errorCode, const QString& errorResponse) { reject(errorResponse); };
            auto responseHandler = [=](const QByteArray& response) { resolve(); };

            QVariantMap body;
            body["fromImage"] = name;
            body["tag"] = tag;

            _requestHandler->makePostRequest(BuildUrl("/images/create", body).toString(),
                QJsonDocument{}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto DockerApiClient::createContainer(QString label, QVariantMap data) -> Promise<QString>
{
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler
                = [=](int errorCode, const QString& errorResponse) { reject(errorResponse); };
            auto responseHandler = [=](const QByteArray& response) {
                auto root = QJsonDocument::fromJson(response).object();
                resolve(root.value("Id").toString());
            };

            QVariantMap name;
            name["name"] = label;

            _requestHandler->makePostRequest(BuildUrl("/containers/create", name).toString(),
                QJsonDocument(QJsonObject::fromVariantMap(data)), errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto DockerApiClient::listImages() -> Promise<std::vector<QVariantMap>>
{
    return Promise<std::vector<QVariantMap>>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler
                = [=](int errorCode, const QString& errorResponse) { reject(errorResponse); };
            auto responseHandler = [=](const QByteArray& response) {
                std::vector<QVariantMap> result;
                for (auto obj : QJsonDocument::fromJson(response).array()) {
                    result.emplace_back(obj.toObject().toVariantMap());
                }
                resolve(result);
            };

            _requestHandler->makeGetRequest(
                "/images/json", QVariantMap{}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto DockerApiClient::listContainers(QString label) -> Promise<std::vector<QVariantMap>>
{
    return Promise<std::vector<QVariantMap>>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler
                = [=](int errorCode, const QString& errorResponse) { reject(errorResponse); };
            auto responseHandler = [=](const QByteArray& response) {
                std::vector<QVariantMap> result;
                for (auto obj : QJsonDocument::fromJson(response).array()) {
                    result.emplace_back(obj.toObject().toVariantMap());
                }
                resolve(result);
            };

            QVariantMap filter;
            filter["all"] = true;
            filter["filters"] = QString("{\"name\":[\"%1\"]}").arg(label);

            _requestHandler->makeGetRequest(BuildUrl("/containers/json", filter).toString(),
                QVariantMap{}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto DockerApiClient::inspect(QString id) -> Promise<QVariantMap>
{
    return Promise<QVariantMap>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) {
                auto result = QJsonDocument::fromJson(response).object().toVariantMap();
                resolve(result);
            };
            _requestHandler->makeGetRequest(QString("/containers/%1/json").arg(id), QVariantMap{},
                errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto DockerApiClient::runContainer(QString id) -> Promise<void>
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(); };

            _requestHandler->makePostRequest(QString("/containers/%1/start").arg(id),
                QJsonDocument{}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto DockerApiClient::stopContainer(QString id) -> Promise<void>
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(); };
            _requestHandler->makePostRequest(QString("/containers/%1/stop").arg(id),
                QJsonDocument{}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto DockerApiClient::info() -> Promise<QVariantMap>
{
    return Promise<QVariantMap>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) {
                resolve(QJsonDocument::fromJson(response).object().toVariantMap());
            };
            _requestHandler->makeGetRequest("/info", QVariantMap{}, errorHandler, responseHandler);
        });
    });
}

//==============================================================================

auto DockerApiClient::removeContainer(QString id, bool force) -> Promise<void>
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto errorHandler = [=](int errorCode, const QString& errorResponse) {
                reject(NetworkUtils::ApiErrorException{ errorCode, errorResponse });
            };

            auto responseHandler = [=](const QByteArray& response) { resolve(); };

            QVariantMap body;
            body["force"] = force;

            _requestHandler->makeDeleteRequest(
                BuildUrl(QString("/containers/%1").arg(id), body).toString(), QJsonDocument{},
                errorHandler, responseHandler);
        });
    });
}

//==============================================================================
