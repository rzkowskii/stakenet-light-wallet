#include "LocalSocketRequestHandlerImpl.hpp"
#include <Networking/NetworkingUtils.hpp>
#include <Utils/Logging.hpp>
#include <Utils/Utils.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalSocket>
#include <boost/beast.hpp>
#include <unordered_map>

namespace http = boost::beast::http; // from <boost/beast/http.hpp>

//==============================================================================

const uint32_t DEFAULT_CACHE_EXPIRY = 90 * 1000; // 90 seconds
const uint32_t DEFAULT_REQUEST_TIMEOUT = 30 * 1000;

//==============================================================================

static QString DockerRemoteAPIEndpoint()
{
#ifdef Q_OS_UNIX
    return "/var/run/docker.sock";
#else
    return "\\\\.\\pipe\\docker_engine";
#endif
}

//==============================================================================

static QByteArray PrepareRawHttpPayload(QString target, http::verb verb, QJsonObject payload = {})
{
    // Set up an HTTP GET request message
    http::request<http::string_body> req{ verb, "1.1", 11 };
    req.set(http::field::host, "localhost");
    req.target(QString("/v1.40%1").arg(target).toStdString());
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
    //    ..set(http::field::accept, "*/*");
    if (!payload.isEmpty()) {
        auto content = QJsonDocument(payload).toJson().toStdString();
        req.set(http::field::content_length, content.size());
        req.set(http::field::content_type, "application/json");
        req.body().append(content);
    }

    std::ostringstream oss;
    oss << req;
    LogCDebug(Connext) << "Sending local socket request:" << oss.str().data();
    return QByteArray::fromStdString(oss.str());
}

//==============================================================================

struct LocalSocketRequestHandlerImpl::HttpState {
    explicit HttpState() {}

    void startPreparedRequest(QLocalSocket* socket, RequestHandler::ResponseHandler success,
        RequestHandler::NetworkErrorHandler fail)
    {
        auto req = std::make_unique<InProgressRequest>();
        req->parser.eager(true);
        req->success = success;
        req->fail = fail;
        _inProgressRequest.emplace(socket, std::move(req));
        socket->connectToServer(DockerRemoteAPIEndpoint());
    }

    QLocalSocket* prepareRequest(http::verb verb, QString target, QJsonObject payload = {})
    {
        static const auto dockerEndpoint = DockerRemoteAPIEndpoint();
        auto rawHttpPayload = PrepareRawHttpPayload(target, verb, payload);
        auto* localSocket = new QLocalSocket;
        localSocket->setProperty("target", target);
        QObject::connect(localSocket, &QLocalSocket::connected, localSocket,
            [rawHttpPayload, localSocket] { localSocket->write(rawHttpPayload); });
        QObject::connect(localSocket, &QLocalSocket::errorOccurred, localSocket, [](auto reason) {
            LogCCritical(Connext) << "Error occurred when connecting to endpoint:" << dockerEndpoint
                                  << reason;
        });
        return localSocket;
    }

    void processTimeout(QLocalSocket* socket)
    {
        if (_inProgressRequest.count(socket) == 0) {
            return;
        }

        auto& state = *_inProgressRequest.at(socket);
        LogCDebug(Connext) << "Failed to wait for more data when it was needed:" << state.consumed
                           << state.total;
        dispatchError(socket, NetworkUtils::ApiErrorException(503, "Data timeout"));
    }

    void dispatchError(QLocalSocket* socket, NetworkUtils::ApiErrorException ex)
    {
        if (_inProgressRequest.count(socket) == 0) {
            return;
        }

        auto& state = *_inProgressRequest.at(socket);
        state.fail(ex.errorCode, ex.errorResponse);

        socket->deleteLater();
        _inProgressRequest.erase(socket);
    }

    void dispatchResponse(QLocalSocket* socket)
    {
        if (_inProgressRequest.count(socket) == 0) {
            return;
        }

        auto& state = *_inProgressRequest.at(socket);
        auto storeInCache = [&state, socket, this] {
            auto msg = state.parser.get();
            auto target = socket->property("target").toString();
            if (msg.result_int() >= 200 && msg.result_int() < 300) {
                _cache[target] = { msg, QDateTime::currentSecsSinceEpoch() };
            } else if (msg.result_int() == 304) {
                auto hasCacheEntry = _cache.count(target) > 0;
                LogCDebug(Connext) << "Recv 304, looking up in cache:" << hasCacheEntry;
                if (hasCacheEntry) {
                    return std::get<0>(_cache.at(target));
                }
            }

            return msg;
        };

        auto msg = storeInCache();
        try {
            auto bodyAsString = boost::beast::buffers_to_string(msg.body().data());
            if (msg.result_int() >= 200 && msg.result_int() < 300) {
                state.success(QByteArray::fromStdString(bodyAsString));
            } else {
                state.fail(msg.result_int(), QString::fromStdString(msg.reason().to_string()));
            }
        } catch (NetworkUtils::ApiErrorException& ex) {
            state.fail(ex.errorCode, ex.errorResponse);
        } catch (std::exception& ex) {
            state.fail(503, ex.what());
        }

        socket->deleteLater();
        _inProgressRequest.erase(socket);
    }

    void processResponse(QLocalSocket* socket)
    {
        if (_inProgressRequest.count(socket) == 0) {
            return;
        }

        auto& state = *_inProgressRequest.at(socket);
        auto payload = socket->readAll();
        LogCDebug(Connext) << "Recv local socket response:" << payload;
        state.total += payload.size();
        state.data.append(payload);

        boost::system::error_code ec;
        if (!state.parser.is_done()) {
            auto buffer = boost::asio::buffer(state.data.data(), state.data.size());
            auto bytesConsumed = state.parser.put(buffer, ec);
            state.data.remove(0, bytesConsumed);
            state.consumed += bytesConsumed;
            LogCDebug(Connext) << "Consumed:" << state.consumed << state.total;
            if (!state.parser.is_done() || (ec && ec == http::error::need_more)) {
                LogCDebug(Connext) << "waiting for data:" << ec.message().data();
                return;
            } else if (ec) {
                LogCCritical(Connext) << "Http parsing error:" << ec.message().data();
                dispatchError(
                    socket, NetworkUtils::ApiErrorException(503, "Failed to parse http response"));
                return;
            }
        }

        if (state.parser.is_done()) {
            dispatchResponse(socket);
        }
    }

    void cleanupCache()
    {
        auto currentTs = QDateTime::currentSecsSinceEpoch();
        for (auto it = _cache.begin(); it != _cache.end();) {
            auto [_, timestamp] = it->second;
            if (currentTs < timestamp + DEFAULT_CACHE_EXPIRY) {
                it = _cache.erase(it);
            } else {
                ++it;
            }
        }
    }

    struct InProgressRequest {
        http::response_parser<http::dynamic_body> parser;
        QByteArray data;
        RequestHandler::ResponseHandler success;
        RequestHandler::NetworkErrorHandler fail;
        uint32_t consumed{ 0 };
        uint32_t total{ 0 };
    };

    using Response = std::tuple<http::response<http::dynamic_body>,
        int64_t>; // data and timestamp till response is valid
    std::unordered_map<QString, Response> _cache; // target -> body
    std::unordered_map<QLocalSocket*, std::unique_ptr<InProgressRequest>> _inProgressRequest;
};

//==============================================================================

LocalSocketRequestHandlerImpl::LocalSocketRequestHandlerImpl(QObject* parent)
    : RequestHandler(parent)
    , _state(new HttpState)
{
    auto cleanupTimer = new QTimer(this);
    connect(cleanupTimer, &QTimer::timeout, this, [this] { _state->cleanupCache(); });
    cleanupTimer->setInterval(DEFAULT_CACHE_EXPIRY);
    cleanupTimer->setSingleShot(false);
    cleanupTimer->start();
}

//==============================================================================

LocalSocketRequestHandlerImpl::~LocalSocketRequestHandlerImpl() {}

//==============================================================================

void LocalSocketRequestHandlerImpl::makeGetRequest(const QString& path, const QVariantMap& params,
    RequestHandler::NetworkErrorHandler errorHandler,
    RequestHandler::ResponseHandler responseHandler, int retryAttempts)
{
    RequestMaker requestMaker
        = [path, this]() { return _state->prepareRequest(http::verb::get, path); };

    processRequest(requestMaker, errorHandler, responseHandler, retryAttempts);
}

//==============================================================================

void LocalSocketRequestHandlerImpl::makePostRequest(const QString& path, const QJsonDocument& body,
    RequestHandler::NetworkErrorHandler errorHandler,
    RequestHandler::ResponseHandler responseHandler, int retryAttempts)
{
    auto payload = body.object();
    RequestMaker requestMaker = [path, payload, this]() {
        return _state->prepareRequest(http::verb::post, path, payload);
    };
    processRequest(requestMaker, errorHandler, responseHandler, retryAttempts);
}

//==============================================================================

void LocalSocketRequestHandlerImpl::makePutRequest(const QString& path, const QVariantMap& params,
    RequestHandler::NetworkErrorHandler errorHandler,
    RequestHandler::ResponseHandler responseHandler, int retryAttempts)
{
    RequestMaker requestMaker = [this, path, params]() {
        return _state->prepareRequest(http::verb::put, path, QJsonObject::fromVariantMap(params));
    };

    processRequest(requestMaker, errorHandler, responseHandler, retryAttempts);
}

//==============================================================================

void LocalSocketRequestHandlerImpl::makeDeleteRequest(const QString& path,
    const QJsonDocument& body, RequestHandler::NetworkErrorHandler errorHandler,
    RequestHandler::ResponseHandler responseHandler, int retryAttempts)
{
    auto payload = body.object();
    RequestMaker requestMaker = [this, path, payload]() {
        return _state->prepareRequest(http::verb::delete_, path, payload);
    };
    processRequest(requestMaker, errorHandler, responseHandler, retryAttempts);
}

//==============================================================================

void LocalSocketRequestHandlerImpl::processRequest(
    LocalSocketRequestHandlerImpl::RequestMaker requestMaker,
    RequestHandler::NetworkErrorHandler errorHandler,
    RequestHandler::ResponseHandler responseHandler, int retryAttempts)
{
    auto socket = requestMaker();
    QTimer* timeout = new QTimer{ this };
    connect(socket, &QLocalSocket::readyRead, this, [=] {
        timeout->start(DEFAULT_REQUEST_TIMEOUT);
        _state->processResponse(socket);
    });
    connect(socket, &QLocalSocket::errorOccurred, this, [socket, this](auto socketError) {
        _state->dispatchError(
            socket, NetworkUtils::ApiErrorException(socketError, socket->errorString()));
    });
    connect(timeout, &QTimer::timeout, this, [this, socket] { _state->processTimeout(socket); });
    connect(socket, &QObject::destroyed, timeout, &QObject::deleteLater);
    timeout->start(DEFAULT_REQUEST_TIMEOUT);

    _state->startPreparedRequest(socket, responseHandler, errorHandler);
}

//==============================================================================
