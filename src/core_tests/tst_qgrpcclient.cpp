#include <GRPCTools/ClientUtils.hpp>
#include <Utils/Utils.hpp>
#include <gen-grpc/tesgrpcserver.grpc.pb.h>
#include <gtest/gtest.h>

#include <QDateTime>
#include <QSemaphore>
#include <QSignalSpy>
#include <chrono>
#include <grpcpp/grpcpp.h>

using namespace testing;
using namespace std::chrono_literals;

class TestServer : public test::TestService::Service {
    // Service interface
public:
    grpc::Status UnaryCall(grpc::ServerContext* context, const test::RequestCall* request,
        test::ResponseCall* response) override
    {
        response->set_requestid(request->requestid());
        std::cout << "Server UnaryCall: " << QDateTime::currentMSecsSinceEpoch() << std::endl;
        return grpc::Status{};
    }

    grpc::Status UnidirectionaStreamingCall(grpc::ServerContext* context,
        const test::RequestUniStreamingCall* request,
        ::grpc::ServerWriter<test::ResponseUniStreamingCall>* writer) override
    {
        auto id = request->requestid();
        auto count = request->numberofevents();

        test::ResponseUniStreamingCall resp;
        resp.set_requestid(id);
        for (int i = 0; i < count; ++i) {
            resp.set_eventindex(i);
            writer->Write(resp);
            //            std::this_thread::sleep_for(100ms);
        }

        //        resp.set_eventindex(count - 1);
        //        writer->WriteLast(resp, grpc::WriteOptions{}.set_last_message());

        return grpc::Status{};
    }

    grpc::Status BidirectionalStreamingCall(grpc::ServerContext* context,
        ::grpc::ServerReaderWriter<test::ResponseUniStreamingCall, test::RequestUniStreamingCall>*
            stream) override
    {
        std::queue<test::RequestUniStreamingCall> requests;
        std::atomic_bool stop{ false };
        std::mutex lock;
        std::thread reader([&] {
            test::RequestUniStreamingCall request;
            while (stream->Read(&request)) {
                if (request.numberofevents() == 0) {
                    break;
                }

                std::lock_guard<std::mutex> guard(lock);
                requests.push(request);
            }
            stop = true;
        });

        std::thread writer([&] {
            while (!stop) {
                std::this_thread::sleep_for(100ms);
                if (stop) {
                    break;
                }

                test::RequestUniStreamingCall request;
                {
                    std::lock_guard<std::mutex> guard(lock);
                    if (requests.empty()) {
                        continue;
                    }
                    request = requests.front();
                    requests.pop();
                }

                test::ResponseUniStreamingCall response;
                response.set_requestid(request.requestid());
                if (!stream->Write(response)) {
                    break;
                }
            }
        });

        reader.join();
        writer.join();
        return grpc::Status{};
    }
};

class QGrpcClientTests : public ::testing::Test {
protected:
    std::unique_ptr<Utils::WorkerThread> _worker;
    std::unique_ptr<grpc::Server> _server;
    QString rpcChannel{ "0.0.0.0:50051" };
    std::unique_ptr<BaseGrpcClient> _client;
    std::unique_ptr<test::TestService::Stub> _stub;

    void SetUp() override
    {
        _worker.reset(new Utils::WorkerThread);
        _worker->start();
        startServer();
        _stub = test::TestService::NewStub(
            grpc::CreateChannel(rpcChannel.toStdString(), grpc::InsecureChannelCredentials()));
        _client = std::make_unique<BaseGrpcClient>(rpcChannel, [] { return std::string{}; },
            [] { return std::string{}; }, BaseGrpcClient::AuthType::None);
        _client->connect();
    }

    void TearDown() override
    {
        stopServer();
        _worker->quit();
        _worker->wait();
        _worker.reset();
        _stub.reset();
        _client.reset();
    }

    void startServer()
    {
        QSemaphore sem;
        QMetaObject::invokeMethod(_worker->context(), [this, &sem] {
            TestServer service;
            grpc::ServerBuilder builder;
            builder.AddListeningPort(rpcChannel.toStdString(), grpc::InsecureServerCredentials());
            builder.RegisterService(&service);
            _server = builder.BuildAndStart();
            sem.release();
            _server->Wait();
        });

        sem.acquire();
    }

    void stopServer() { _server->Shutdown(); }
};

TEST_F(QGrpcClientTests, UnaryCallSuccess)
{
    auto stub = test::TestService::NewStub(
        grpc::CreateChannel(rpcChannel.toStdString(), grpc::InsecureChannelCredentials()));
    ClientContext ctx;
    test::RequestCall request;
    test::ResponseCall response;
    request.set_requestid("1");
    std::cout << "Client Before UnaryCall: " << QDateTime::currentMSecsSinceEpoch() << std::endl;
    auto status = stub->UnaryCall(&ctx, request, &response);
    std::cout << "Client After UnaryCall: " << QDateTime::currentMSecsSinceEpoch() << std::endl;
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(request.requestid(), response.requestid());
}

TEST_F(QGrpcClientTests, UnaryCallAsyncSuccess)
{
    test::RequestCall request;
    request.set_requestid("1");
    std::cout << "Client UnaryCall: " << QDateTime::currentMSecsSinceEpoch() << std::endl;
    _client
        ->makeUnaryRequest<test::ResponseCall>(
            _stub.get(), &test::TestService::Stub::PrepareAsyncUnaryCall, request, 0)
        .then([request](test::ResponseCall response) {
            ASSERT_EQ(response.requestid(), request.requestid());
        })
        .wait();
}

TEST_F(QGrpcClientTests, StreamingReadAsyncCallSuccess)
{
    test::RequestUniStreamingCall request;
    request.set_requestid("1");
    request.set_numberofevents(300);
    std::vector<test::ResponseUniStreamingCall> items;
    QObject context;

    bool closed = false;
    auto readContext = ObserveAsync<test::ResponseUniStreamingCall>(&context,
        [&items](auto item) { items.emplace_back(item); },
        [&closed](grpc::Status status) { closed = status.ok(); });

    _client->makeStreamingReadRequest(_stub.get(),
        &test::TestService::Stub::PrepareAsyncUnidirectionaStreamingCall, request,
        std::move(readContext), 0);

    QSignalSpy spy(&context, &QObject::destroyed);
    spy.wait(1000);

    ASSERT_TRUE(closed);
    ASSERT_EQ(request.numberofevents(), items.size());
}

TEST_F(QGrpcClientTests, StreamingConnectPromise)
{
    Utils::WorkerThread extraWorker;
    extraWorker.start();
    QObject context;
    QObject* contextPtr = &context;
    ASSERT_FALSE(QtPromise::QPromise<void>(
        [this, contextPtr, &extraWorker](const auto& resolve, const auto& reject) {
            QMetaObject::invokeMethod(extraWorker.context(), [=] {
                test::RequestUniStreamingCall request;
                request.set_requestid("1");
                request.set_numberofevents(1);

                auto readContext = ObserveAsync<test::ResponseUniStreamingCall>(
                    contextPtr, [resolve](auto) { resolve(); }, [reject](auto) { reject(); });

                _client->makeStreamingReadRequest(_stub.get(),
                    &test::TestService::Stub::PrepareAsyncUnidirectionaStreamingCall, request,
                    std::move(readContext), 0);
            });
        })
                     .wait()
                     .isRejected());
}

#if 0
TEST_F(QGrpcClientTests, StreamingReadWriteAsyncCallSuccess)
{
    const size_t numberOfRequests = 3;
    test::RequestUniStreamingCall request;
    request.set_requestid("1");
    request.set_numberofevents(1);
    StreamingReadWriteContext<test::RequestUniStreamingCall, test::ResponseUniStreamingCall>
        readWriteContext;
    std::vector<test::ResponseUniStreamingCall> items;
    QObject::connect(&readWriteContext,
        &StreamingReadWriteContext<test::RequestUniStreamingCall,
            test::ResponseUniStreamingCall>::readyRead,
        [&] {
            for (auto&& item : readWriteContext.takeItems()) {
                items.emplace_back(item);
            }
        });
    QSignalSpy spy(&readWriteContext,
        &StreamingReadWriteContext<test::RequestUniStreamingCall,
            test::ResponseUniStreamingCall>::closed);
    _client->makeStreamingReadWriteRequest(_stub.get(),
        &test::TestService::Stub::PrepareAsyncBidirectionalStreamingCall, &readWriteContext, 0);

    for (size_t i = 0; i < numberOfRequests; ++i) {
        request.set_requestid(std::to_string(i));
        readWriteContext.write(request);
    }

    std::this_thread::sleep_for(1000ms); // wait for response and process events

    request.set_numberofevents(0);
    readWriteContext.write(request); // stop from server side

    QCoreApplication::processEvents();

    ASSERT_TRUE(spy.count() > 0 || spy.wait(10000));
    ASSERT_EQ(numberOfRequests, items.size());
}


TEST_F(QGrpcClientTests, StreamingReadWriteAsyncCallStop)
{
    const size_t numberOfRequests = 3;
    test::RequestUniStreamingCall request;
    request.set_requestid("1");
    request.set_numberofevents(1);
    StreamingReadWriteContext<test::RequestUniStreamingCall, test::ResponseUniStreamingCall>
        readWriteContext;
    std::vector<test::ResponseUniStreamingCall> items;
    QObject::connect(&readWriteContext,
        &StreamingReadWriteContext<test::RequestUniStreamingCall,
            test::ResponseUniStreamingCall>::readyRead,
        [&] {
            for (auto&& item : readWriteContext.takeItems()) {
                items.emplace_back(item);
            }
        });
    QSignalSpy spy(&readWriteContext,
        &StreamingReadWriteContext<test::RequestUniStreamingCall,
            test::ResponseUniStreamingCall>::closed);
    _client->makeStreamingReadWriteRequest(_stub.get(),
        &test::TestService::Stub::PrepareAsyncBidirectionalStreamingCall, &readWriteContext, 0);

    for (size_t i = 0; i < numberOfRequests; ++i) {
        request.set_requestid(std::to_string(i));
        readWriteContext.write(request);
    }

    _client.reset();

    QCoreApplication::processEvents();
}


TEST_F(QGrpcClientTests, UnaryCallAsyncSuccess)
{
    auto channel = grpc::CreateChannel(rpcChannel.toStdString(), grpc::InsecureChannelCredentials());
    auto stub = test::TestService::NewStub(channel);
    ClientContext ctx;
    test::RequestCall request;
    test::ResponseCall response;
    request.set_requestid("1");
    QSemaphore sem;
    grpc::Status status;

    struct UnaryReactor : grpc::experimental::ClientUnaryReactor {
        UnaryReactor(QSemaphore &sema, grpc::Status &status) :
            sema(sema),
            status(status)
        {
        }
        virtual void OnDone(const ::grpc::Status& s) override
        {
            std::cout << "Client onDone: " << QDateTime::currentMSecsSinceEpoch() << std::endl;
            status = s;
            sema.release();
        }
        virtual void OnReadInitialMetadataDone(bool /*ok*/) override
        {

        }

        QSemaphore &sema;
        grpc::Status &status;
    };

    UnaryReactor reactor(sem, status);

    std::cout << "Client Before UnaryCall: " << QDateTime::currentMSecsSinceEpoch() << std::endl;
    stub->experimental_async()->UnaryCall(&ctx, &request, &response, &reactor);
    reactor.StartCall();
    sem.acquire();
    std::cout << "Client After acquire: " << QDateTime::currentMSecsSinceEpoch() << std::endl;
    ASSERT_TRUE(status.ok());
    ASSERT_EQ(request.requestid(), response.requestid());
}

TEST_F(QGrpcClientTests, StreamingCallAsyncSuccess)
{
    auto channel = grpc::CreateChannel(rpcChannel.toStdString(), grpc::InsecureChannelCredentials());
    auto stub = test::TestService::NewStub(channel);
    ClientContext ctx;
    test::RequestUniStreamingCall request;
    request.set_requestid("1");
    request.set_numberofevents(1);
    QSemaphore sem;
    grpc::Status status;

    struct StreamingReactor : grpc::experimental::ClientReadReactor<test::ResponseUniStreamingCall> {
        StreamingReactor(QSemaphore &sema, grpc::Status &status) :
            sema(sema),
            status(status)
        {

        }
        virtual void OnDone(const ::grpc::Status& s) override
        {
            status = s;
            sema.release();
        }
        virtual void OnReadDone(bool ok) override
        {
            if(ok)
            {
                StartRead(&response);
            }
        }
        virtual void OnReadInitialMetadataDone(bool /*ok*/) override
        {

        }

        test::ResponseUniStreamingCall response;
        QSemaphore &sema;
        grpc::Status &status;
    };

    StreamingReactor reactor(sem, status);

    stub->experimental_async()->UnidirectionaStreamingCall(&ctx, &request, &reactor);
    reactor.StartCall();
    reactor.StartRead(&reactor.response);
    sem.acquire();
    ASSERT_TRUE(status.ok());
}
#endif
