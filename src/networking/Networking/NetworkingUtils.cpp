#include "NetworkingUtils.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <Utils/Logging.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

//==============================================================================

static QNetworkAccessManager* GetStaticAccessManager()
{
    static QNetworkAccessManager accessManager;
    return &accessManager;
}

//==============================================================================

QtPromise::QPromise<QByteArray> NetworkUtils::downloadFile(
    QNetworkAccessManager* accessManager, QUrl downloadUrl, ProgressHandler onProgress)
{
    return QtPromise::QPromise<QByteArray>(
        [accessManager, downloadUrl, onProgress](const auto& resolve, const auto& reject) {
            QNetworkRequest request(downloadUrl);
            auto reply = accessManager->get(request);
            if (onProgress) {
                QObject::connect(reply, &QNetworkReply::downloadProgress, onProgress);
            }

            QtPromise::connect(reply, &QNetworkReply::finished)
                .then([reply, resolve, reject, downloadUrl]() {
                    //            LogCDebug(General) << "Finished file downloading" << downloadUrl
                    //            << reply->error();
                    if (reply->error() != QNetworkReply::NoError) {
                        reject(reply->errorString());
                    } else {
                        resolve(reply->readAll());
                    }
                })
                .finally([reply] { reply->deleteLater(); });
        });
}

//==============================================================================

QtPromise::QPromise<QByteArray> NetworkUtils::downloadFile(
    QUrl downloadUrl, ProgressHandler onProgress)
{
    return downloadFile(GetStaticAccessManager(), downloadUrl, onProgress);
}

//==============================================================================


NetworkUtils::ApiErrorException::ApiErrorException(
    int errorCode, QString errorString)
    : errorCode(errorCode)
    , errorResponse(errorString)
{
    for (auto value :
        QJsonDocument::fromJson(errorString.toLatin1()).object().value("errors").toArray()) {
        auto obj = value.toObject();
        errors.emplace_back(obj.value("type").toString(), obj.value("field").toString(),
            obj.value("message").toString());
    }
}

//==============================================================================

void NetworkUtils::ApiErrorException::raise() const
{
    throw *this;
}

//==============================================================================

QException* NetworkUtils::ApiErrorException::clone() const
{
    return new ApiErrorException(*this);
}

//==============================================================================
