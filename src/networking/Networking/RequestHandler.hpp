#ifndef REQUESTHANDLER_HPP
#define REQUESTHANDLER_HPP

#include <QObject>
#include <QVariantMap>
#include <functional>

class RequestHandler : public QObject {
    Q_OBJECT
public:
    using ResponseHandler = std::function<void(const QByteArray&)>;
    using NetworkErrorHandler = std::function<void(int, const QString&)>;

public:
    explicit RequestHandler(QObject* parent = nullptr);

    virtual void makeGetRequest(const QString& path, const QVariantMap& params,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler, int retryAttempts = 0)
        = 0;

    // post as json
    virtual void makePostRequest(const QString& path, const QJsonDocument& body,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler, int retryAttempts = 0)
        = 0;

    // delete as json
    virtual void makeDeleteRequest(const QString& path, const QJsonDocument& body,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler, int retryAttempts = 0)
        = 0;

    virtual void makePutRequest(const QString& path, const QVariantMap& params,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler, int retryAttempts = 0)
        = 0;
};

#endif // REQUESTHANDLER_HPP
