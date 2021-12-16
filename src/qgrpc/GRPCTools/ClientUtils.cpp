#include "ClientUtils.hpp"

#include <grpcpp/security/credentials_impl.h>

const grpc::Status BaseGrpcClient::notConnectedStatus(grpc::StatusCode::UNKNOWN, "Not connected");

struct MacaroonAuthPlugin : public grpc::MetadataCredentialsPlugin {
    explicit MacaroonAuthPlugin(std::string macaroon)
        : _macaroon(macaroon)
    {
    }
    // MetadataCredentialsPlugin interface
public:
    grpc::Status GetMetadata(grpc::string_ref service_url, grpc::string_ref method_name,
        const grpc::AuthContext& channel_auth_context,
        std::multimap<grpc::string, grpc::string>* metadata) override
    {
        Q_UNUSED(service_url);
        Q_UNUSED(method_name);
        Q_UNUSED(channel_auth_context);
        metadata->emplace("macaroon", _macaroon);
        return grpc::Status::OK;
    }

    std::string _macaroon;
};

BaseStreamingContext::BaseStreamingContext(QObject* parent)
    : QObject(parent)
{
    static std::once_flag once;
    std::call_once(once, [] { qRegisterMetaType<grpc::Status>("grpc::Status"); });
}

void BaseStreamingContext::notifyClosed(grpc::Status status)
{
    QMetaObject::invokeMethod(this, [this, status] { closed(status); });
}

BaseGrpcClient::BaseGrpcClient(QString rpcChannel, TlsCertProvider tlsCertProvider,
    MacaroonProvider macaroonProvider, AuthType authType, QObject* parent)
    : QObject(parent)
    , _rpcChannel(rpcChannel)
    , _tlsCertProvider(tlsCertProvider)
    , _macaroonProvider(macaroonProvider)
    , _authType(authType)
{
	LogCDebug(GRPC) << "Creating grpc client" << reinterpret_cast<uintptr_t>(this);
    startWorker();
}

std::unique_ptr<grpc::ClientContext> BaseGrpcClient::CreateContext(size_t timeoutMs)
{
    std::unique_ptr<ClientContext> context(new ClientContext);

    if (timeoutMs > 0) {
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(timeoutMs);
        context->set_deadline(deadline);
    }

    return context;
}

BaseGrpcClient::~BaseGrpcClient()
{
	LogCDebug(GRPC) << "Detroying grpc client" << this;
    _isShutdown = true;
    _cq.Shutdown();
    _worker.quit();
    _worker.wait();
}

void BaseGrpcClient::tearDown()
{
    _channel.reset();
}

bool BaseGrpcClient::init()
{
    if (!_channel) {

#ifdef Q_OS_WIN
        qputenv("GRPC_DEFAULT_SSL_ROOTS_FILE_PATH",
            QDir(QCoreApplication::applicationDirPath())
                .absoluteFilePath("share/grpc/root.pem")
                .toUtf8());
#endif
        grpc::SslCredentialsOptions tlsOptions;
        tlsOptions.pem_root_certs = _tlsCertProvider();

        const bool needsSsl = _authType == AuthType::SSL || _authType == AuthType::Macaroon;

        if (needsSsl && tlsOptions.pem_root_certs.empty()) {
            return false;
        }

        auto channelCreds
            = needsSsl ? grpc::SslCredentials(tlsOptions) : grpc::InsecureChannelCredentials();

        if (_authType == AuthType::Macaroon) {
            auto macaroon = _macaroonProvider();
            if (macaroon.empty()) {
                return false;
            }
            auto macaroonCreds = grpc::MetadataCredentialsFromPlugin(
                std::make_unique<MacaroonAuthPlugin>(macaroon));
            channelCreds = grpc::CompositeChannelCredentials(channelCreds, macaroonCreds);
        }
        _channel = grpc::CreateChannel(_rpcChannel.toStdString(), channelCreds);
    }

    return true;
}

void BaseGrpcClient::connect()
{
    if (!init()) {
        throw std::runtime_error("Client not initialized");
    }
}

uint64_t BaseGrpcClient::generateId()
{
    auto advanceId = [this](uint64_t& outId) { outId = ++_lastUsedId; };

    uint64_t id;
    do {
        advanceId(id);
    } while (_pendingCalls.count(id) > 0);

    return id;
}

BaseGrpcClient::CallOps* BaseGrpcClient::generateCallOps(uint64_t id, std::string payload)
{
    CallOps* ops = new CallOps;
    ops->id = id;
    ops->payload = payload;
    return ops;
}

void BaseGrpcClient::completeCall(uint64_t id)
{
    auto it = _pendingCalls.find(id);
    if (it != std::end(_pendingCalls)) {
        auto call = it->second.get();
        if (call->_state == BaseAsyncClientCall::State::Finished) {
            LogCDebug(GRPC) << "Call has finished, erasing" << reinterpret_cast<uintptr_t>(call);
            _pendingCalls.erase(it);
        }
    }
}

void BaseGrpcClient::completeAllCalls()
{
    for (auto&& it : _pendingCalls) {
        auto call = it.second.get();
        call->_state = BaseAsyncClientCall::State::Shutdown;
        call->process(it.first, {});
    }

    _pendingCalls.clear();
}

BaseAsyncClientCall* BaseGrpcClient::pendingCall(uint64_t id)
{
    return _pendingCalls.count(id) > 0 ? _pendingCalls.at(id).get() : nullptr;
}

void BaseGrpcClient::completeRpc()
{
    QMetaObject::invokeMethod(
        _worker.context(), [this] { _semaphore.release(); }, Qt::QueuedConnection);

    // Block until the next result is available in the completion queue "cq".
    while (true) {
        bool ok = false;
        void* got_tag = nullptr;

        std::chrono::system_clock::time_point deadline
            = std::chrono::system_clock::now() + std::chrono::milliseconds(20);
        auto status = _cq.DoThenAsyncNext(
            [] { QCoreApplication::processEvents(); }, &got_tag, &ok, deadline);

        if (_isShutdown) {
            status = CompletionQueue::SHUTDOWN;
        }

        if (status == CompletionQueue::TIMEOUT) {
            continue;
        } else if (status == CompletionQueue::SHUTDOWN) {
            break;
        }

        auto ops = reinterpret_cast<CallOps*>(got_tag);
        if (auto call = pendingCall(ops->id)) {
            call->process(ok, ops->payload);
            completeCall(ops->id);
            delete ops;
        }
    }

    completeAllCalls();
}

void BaseGrpcClient::startWorker()
{
    _worker.rename("Stakenet-GrpcClientWorker");

    if (!_worker.isRunning()) {
        _worker.start();
        QMetaObject::invokeMethod(_worker.context(), [this] { completeRpc(); });

        _semaphore.acquire();
    }
}

void BaseGrpcClient::startPreparedCall(
    uint64_t id, std::unique_ptr<BaseAsyncClientCall> preparedCall)
{
	LogCDebug(GRPC) << "Setting alarm for" << reinterpret_cast<uintptr_t>(preparedCall.get());
	preparedCall->_alarm.Set(
        &_cq, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), preparedCall->_generateTag({}));
    _pendingCalls[id] = std::move(preparedCall);
}
