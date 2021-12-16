#include "LndClient.hpp"
#include <LndTools/Protos/invoices.grpc.pb.h>
#include <LndTools/Protos/rpc.grpc.pb.h>

#include <GRPCTools/ClientUtils.hpp>
#include <QDebug>
#include <QFile>
#include <boost/optional.hpp>
#include <functional>

using grpc::Channel;
using grpc::ClientAsyncReader;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using namespace lnrpc;

static std::unique_ptr<ClientContext> CreateContext(size_t timeoutMs = 2500)
{
    std::unique_ptr<ClientContext> context(new ClientContext);

    if (timeoutMs > 0) {
        auto deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(timeoutMs);
        context->set_deadline(deadline);
    }

    return context;
}

template <class T> struct EventHandlerProxy : public StreamingEventHandler<T> {
    using OnObjectReceived = std::function<void(T)>;
    void onObjectReceived(T object) override
    {
        if (proxy) {
            proxy(object);
        }
    }

    OnObjectReceived proxy;
};

struct LndClient::Impl {
    QString rpcChannel;
    std::string tlsCert;
    std::unique_ptr<Lightning::Stub> client;
    std::unique_ptr<invoicesrpc::Invoices::Stub> invoices;
    StreamingAsyncCall<Invoice>* invoiceSubscription;
    std::shared_ptr<EventHandlerProxy<Invoice>> _invoicesEventHandler;
    bool isShutdown{ false };
    std::unordered_map<uint64_t, std::unique_ptr<BaseAsyncClientCall>> _pendingCalls;

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    CompletionQueue cq;

    uint64_t lastUsedId{ 0 };

    BaseAsyncClientCall* pendingCall(uint64_t id)
    {
        return _pendingCalls.count(id) > 0 ? _pendingCalls.at(id).get() : nullptr;
    }

    void completeCall(uint64_t id)
    {
        auto it = _pendingCalls.find(id);
        if (it != std::end(_pendingCalls)) {
            auto call = it->second.get();
            if (call->_state == BaseAsyncClientCall::State::Finished) {
                _pendingCalls.erase(it);
            }
        }
    }

    void completeAllCalls()
    {
        for (auto&& it : _pendingCalls) {
            auto call = it.second.get();
            call->_state = BaseAsyncClientCall::State::Shutdown;
            call->process(false);
        }

        _pendingCalls.clear();
    }

    template <class T, class F, class Request> Promise<T> makeUnaryRequest(F&& func, Request&& req)
    {
        auto id = ++lastUsedId;
        std::unique_ptr<BaseAsyncClientCall> call(new UnaryAsyncCall<T>(CreateContext(), id));
        auto callRawPtr = call.get();
        _pendingCalls.emplace(id, std::move(call));

        return static_cast<UnaryAsyncCall<T>*>(callRawPtr)
            ->makeRequest(std::bind(func, client.get(), std::placeholders::_1,
                              std::placeholders::_2, std::placeholders::_3),
                req, &cq);
    }

    template <class T, class F, class Request>
    StreamingContext<T>* makeStreamingRequest(F&& func, Request&& req)
    {
        auto id = ++lastUsedId;
        std::unique_ptr<BaseAsyncClientCall> call(new StreamingAsyncCall<T>(CreateContext(0), id));
        auto callRawPtr = call.get();
        _pendingCalls.emplace(id, std::move(call));

        return static_cast<StreamingAsyncCall<T>*>(callRawPtr)
            ->makeRequest(std::bind(func, client.get(), std::placeholders::_1,
                              std::placeholders::_2, std::placeholders::_3),
                req, &cq, nullptr);
    }

    void init()
    {
        grpc::SslCredentialsOptions tlsOptions;
        tlsOptions.pem_root_certs = tlsCert;
        auto channel
            = grpc::CreateChannel(rpcChannel.toStdString(), grpc::SslCredentials(tlsOptions));
        client = Lightning::NewStub(channel);
        invoices = invoicesrpc::Invoices::NewStub(channel);
    }

    void completeRpc()
    {

        // Block until the next result is available in the completion queue "cq".
        while (true) {
            bool ok = false;
            void* got_tag = nullptr;

            std::chrono::system_clock::time_point deadline
                = std::chrono::system_clock::now() + std::chrono::seconds(1);

            auto status = cq.AsyncNext(&got_tag, &ok, deadline);

            if (isShutdown) {
                status = CompletionQueue::SHUTDOWN;
            }

            if (status == CompletionQueue::TIMEOUT) {
                continue;
            } else if (status == CompletionQueue::SHUTDOWN) {
                // The tag in this example is the memory location of the call object
                break;
            }

            auto id = reinterpret_cast<uint64_t>(got_tag);
            if (auto call = pendingCall(id)) {
                call->process(ok);
                completeCall(id);
            }
        }

        completeAllCalls();
    }
};

LndClient::LndClient(QString rpcChannel, std::string tlsCert, QObject* parent)
    : AbstractLndClient(parent)
{
    _impl.reset(new Impl);
    _impl->rpcChannel = rpcChannel;
    _impl->tlsCert = tlsCert;
    _worker.start();
    QMetaObject::invokeMethod(_worker.context(), [this] { _impl->completeRpc(); });
}

LndClient::~LndClient()
{
    _impl->isShutdown = true;
    _impl->cq.Shutdown();
    _worker.quit();
    _worker.wait();
}

AbstractLndClient::AbstractLndClient(QObject* parent)
    : QObject(parent)
{
}

bool LndClient::openConnection()
{
    _impl->init();
    return true;
}

void LndClient::subscribeForInvoices()
{
    auto context = _impl->makeStreamingRequest<Invoice>(
        &Lightning::Stub::PrepareAsyncSubscribeInvoices, InvoiceSubscription());
    connect(context, &StreamingContext<Invoice>::readyRead, this, [context] {
        for (auto&& item : context->takeItems()) {
            qCDebug(Lnd) << "Dispatching notif:" << item.add_index();
        }
    });

    connect(context, &StreamingContext<Invoice>::closed, this,
        [] { qCDebug(Lnd) << "Streaming channel closed"; });
}

void LndClient::getInfo()
{
    _impl->makeUnaryRequest<GetInfoResponse>(
             &Lightning::Stub::PrepareAsyncGetInfo, GetInfoRequest())
        .then([](GetInfoResponse response) { qCDebug(Lnd) << response.block_hash().c_str(); });
}
