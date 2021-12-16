#ifndef REQUESTHANDLERIMPL_HPP
#define REQUESTHANDLERIMPL_HPP

#include <Networking/RequestHandler.hpp>

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QVariantMap>

class NetworkConnectionState;

class RequestHandlerImpl : public RequestHandler {
    Q_OBJECT
public:
    struct Domains {
        QString normal;
        QString tor;
    };

public:
    RequestHandlerImpl(QNetworkAccessManager* networkManager,
        NetworkConnectionState* connectionState, const Domains& domains, QObject* parent = nullptr);
    virtual ~RequestHandlerImpl();

    void makeGetRequest(const QString& path, const QVariantMap& params,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler,
        int retryAttempts = 0) override;

    // post as json
    void makePostRequest(const QString& path, const QJsonDocument& body,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler,
        int retryAttempts = 0) override;

    void makePutRequest(const QString& path, const QVariantMap& params,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler,
        int retryAttempts = 0) override;

    void makeDeleteRequest(const QString& path, const QJsonDocument& body,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler, int retryAttempts = 0)
        override;

    QNetworkAccessManager* networkAccessManager() const;

private slots:
    void processResponse(
        QNetworkReply* reply, NetworkErrorHandler errorHandler, ResponseHandler responseHandler);

private:
    using RequestMaker = std::function<QNetworkReply*()>;
    void processRequest(RequestMaker requestMaker, NetworkErrorHandler errorHandler,
        ResponseHandler responseHandler, int retryAttempts = 1);

    QUrl buildUrl(const QString& path, const QVariantMap& params = QVariantMap());
    void setProxy();

private:
    QNetworkAccessManager* _networkAccessManager{ nullptr };
    NetworkConnectionState* _connectionState{ nullptr };
    Domains _domains;
    bool _useProxy{ false };
};

#endif // REQUESTHANDLERIMPL_HPP
