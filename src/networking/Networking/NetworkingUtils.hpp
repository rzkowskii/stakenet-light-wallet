#ifndef NETWORKINGUTILS_HPP
#define NETWORKINGUTILS_HPP

#include <QByteArray>
#include <QUrl>
#include <QtPromise>
#include <functional>
#include <QException>

class QNetworkAccessManager;
namespace NetworkUtils {
using ProgressHandler = std::function<void(qint64, qint64)>;
QtPromise::QPromise<QByteArray> downloadFile(
    QNetworkAccessManager* accessManager, QUrl downloadUrl, ProgressHandler onProgress = {});
QtPromise::QPromise<QByteArray> downloadFile(QUrl downloadUrl, ProgressHandler onProgress = {});

struct ApiErrorException : public QException {
    struct ApiError {
        ApiError(QString type, QString field, QString message)
            : type(type)
            , field(field)
            , message(message)
        {
        }

        QString type;
        QString field;
        QString message;
    };

    int errorCode;
    QString errorResponse;
    std::vector<ApiError> errors;

    ApiErrorException(int errorCode, QString errorResponse);
    void raise() const override;
    QException* clone() const override;
};
}

#endif // NETWORKINGUTILS_HPP
