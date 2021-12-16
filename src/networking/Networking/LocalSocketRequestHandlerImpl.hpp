#ifndef LOCALSOCKETREQUESTHANDLERIMPL_HPP
#define LOCALSOCKETREQUESTHANDLERIMPL_HPP

#include <Networking/RequestHandler.hpp>

#include <memory>

class QLocalSocket;

class LocalSocketRequestHandlerImpl : public RequestHandler {
    Q_OBJECT
public:
    explicit LocalSocketRequestHandlerImpl(QObject* parent = nullptr);
    ~LocalSocketRequestHandlerImpl();

    // RequestHandler interface
public:
    void makeGetRequest(const QString& path, const QVariantMap& params,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler,
        int retryAttempts) override;
    void makePostRequest(const QString& path, const QJsonDocument& body,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler,
        int retryAttempts) override;
    void makePutRequest(const QString& path, const QVariantMap& params,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler,
        int retryAttempts) override;
    void makeDeleteRequest(const QString& path, const QJsonDocument& body,
        NetworkErrorHandler errorHandler, ResponseHandler responseHandler,
        int retryAttempts = 0) override;

private:
    using RequestMaker = std::function<QLocalSocket*()>;
    void processRequest(RequestMaker requestMaker, NetworkErrorHandler errorHandler,
        ResponseHandler responseHandler, int retryAttempts = 1);

private:
    struct HttpState;
    std::unique_ptr<HttpState> _state;
};

#endif // LOCALSOCKETREQUESTHANDLERIMPL_HPP
