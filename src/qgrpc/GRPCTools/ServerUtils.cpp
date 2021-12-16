#include "ServerUtils.hpp"

//==============================================================================

qgrpc::BaseGrpcServer::BaseGrpcServer(std::string serverAddress)
    : _serverAddress(serverAddress)
{
}

//==============================================================================

qgrpc::BaseGrpcServer::~BaseGrpcServer()
{
    shutdown();
}

//==============================================================================

void qgrpc::BaseGrpcServer::run(std::vector<grpc::Service*> services)
{
    grpc::ServerBuilder builder;
    builder.AddListeningPort(_serverAddress, grpc::InsecureServerCredentials());
    for (auto&& service : services) {
        builder.RegisterService(service);
    }

    _cq = builder.AddCompletionQueue();
    _server = builder.BuildAndStart();

    exec();
}

//==============================================================================

void qgrpc::BaseGrpcServer::shutdown()
{
    if (!_cq) {
        return;
    }

    QMetaObject::invokeMethod(_context, [this] {
        for (auto& call : _calls) {
            call.second->process(false);
        }
    });
    _server->Shutdown();
    _cq->Shutdown();
}

//==============================================================================

void qgrpc::BaseGrpcServer::exec()
{
    _context = new QObject;

    registerService();

    void* tag; // uniquely identifies a request.
    bool ok;
    while (true) {
        std::chrono::system_clock::time_point deadline
            = std::chrono::system_clock::now() + std::chrono::milliseconds(20);
        auto status
            = _cq->DoThenAsyncNext([] { QCoreApplication::processEvents(); }, &tag, &ok, deadline);

        if (status == grpc::CompletionQueue::NextStatus::TIMEOUT) {
            continue;
        } else if (status == grpc::CompletionQueue::NextStatus::SHUTDOWN) {
            break;
        }

        size_t id = reinterpret_cast<uint64_t>(tag);
        if (_calls.count(id) == 0) {
            continue;
        }

        auto& call = _calls.at(id);
        // means we got a connection
        if (ok && call->_state == qgrpc::BaseServerAsyncCall::State::Connecting) {
            // replicate this call to serve other clients, and process current one.
            auto newId = generateId();
            auto newCall = call->replicateAndProcess(newId);
            newCall->process(true);
            _calls.emplace(newId, std::move(newCall));
        }

        call->process(ok);

        if (call->_state == qgrpc::BaseServerAsyncCall::State::Finished) {
            _calls.erase(id);
        }
    }

    _calls.clear();
}

//==============================================================================

uint64_t qgrpc::BaseGrpcServer::generateId()
{
    auto advanceId = [](uint64_t& outId) {
        static uint64_t id = 0;
        outId = ++id;
    };

    uint64_t id;
    do {
        advanceId(id);
    } while (_calls.count(id) > 0);

    return id;
}

//==============================================================================
