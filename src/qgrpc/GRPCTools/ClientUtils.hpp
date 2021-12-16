#ifndef CLIENTUTILS_HPP
#define CLIENTUTILS_HPP

#include <Utils/Logging.hpp>
#include <Utils/Utils.hpp>

#include <QDir>
#include <QSemaphore>
#include <boost/optional.hpp>
#include <cstdint>
#include <grpcpp/alarm.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>
#include <memory>
#include <queue>
#include <unordered_map>
#include <unordered_set>

//==============================================================================

using grpc::Channel;
using grpc::ClientAsyncReader;
using grpc::ClientAsyncReaderWriter;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;

//==============================================================================

template <class T> struct StreamObserver {
    virtual void onNext(const T& value) = 0;
    virtual void onCompleted(grpc::Status status) = 0;
	virtual ~StreamObserver() {}
};

//==============================================================================

template <class Request, class Response> struct StreamRequestObserver : StreamObserver<Response> {
    void write(Request request)
    {
        QMetaObject::invokeMethod(
            _executionContext, [request, this] { _notifyDataReady(request); });
    }

    QObject* _executionContext{ nullptr };
    std::function<void(Request)> _notifyDataReady;
};

//==============================================================================

class BaseStreamingContext : public QObject {
    Q_OBJECT
public:
    explicit BaseStreamingContext(QObject* parent = nullptr);
    void notifyClosed(grpc::Status status);

signals:
    void readyRead();
    void closed(grpc::Status status);
};

//==============================================================================

template <class T>
std::unique_ptr<StreamObserver<T>> ObserveAsync(QPointer<QObject> context,
    std::function<void(T)> onNext, std::function<void(grpc::Status)> onCompleted)
{
    struct Adapter : StreamObserver<T> {
        Adapter(QPointer<QObject> context, std::function<void(T)> onNext,
            std::function<void(grpc::Status)> onCompleted)
            : _context(context)
            , _onNext(onNext)
            , _onCompleted(onCompleted)
        {
			LogCDebug(GRPC) << "Created streaming observer" << reinterpret_cast<uintptr_t>(this);
        }

		~Adapter() 
		{
			LogCDebug(GRPC) << "Destroying streaming observer" << reinterpret_cast<uintptr_t>(this);
		}

        void onNext(const T& value) override
        {
            if (_context) {
                auto next = _onNext;
                auto context = _context;
				LogCDebug(GRPC) << "Preparing to dispatch onNext" << reinterpret_cast<uintptr_t>(this) 
					<<  "for context:" << context;
                QMetaObject::invokeMethod(context, [next, context, value] {
                    if (context) {
						LogCDebug(GRPC) << "Dispatching onNext" << context;
                        next(value);
                    }
                });
            }
        }

        void onCompleted(grpc::Status status) override
        {
            if (_context) {
                auto context = _context;
                auto completed = _onCompleted;
				LogCDebug(GRPC) << "Preparing to dispatch onCompleted" << reinterpret_cast<uintptr_t>(this)
					<< "for context:" << context;
                QMetaObject::invokeMethod(context, [completed, context, status] {
                    if (context) {
						LogCDebug(GRPC) << "Dispatching onCompleted" << context;
                        completed(status);
                    }
                });
            }
        }

        QPointer<QObject> _context;
        std::function<void(T)> _onNext;
        std::function<void(grpc::Status)> _onCompleted;
    };

    return std::make_unique<Adapter>(context, onNext, onCompleted);
}

//==============================================================================

template <class T> class StreamingReadContext : public BaseStreamingContext {

public:
    explicit StreamingReadContext(QObject* parent = nullptr)
        : BaseStreamingContext(parent)
    {
    }

    void notifyReadyRead(T item)
    {
        QMetaObject::invokeMethod(this, [=] {
            _items.emplace_back(item);
            readyRead();
        });
    }

    std::vector<T> takeItems()
    {
        std::vector<T> result;
        result.swap(_items);
        return result;
    }

private:
    std::vector<T> _items;
};

//==============================================================================

template <class Request, class Response>
class StreamingReadWriteContext : public StreamingReadContext<Response> {

public:
    explicit StreamingReadWriteContext(QObject* parent = nullptr)
        : StreamingReadContext<Response>(parent)
    {
    }

    void write(Request value)
    {
        Q_ASSERT(_executionContext);
        Q_ASSERT(_notifyDataReady);
        QMetaObject::invokeMethod(_executionContext, [this, value] { _notifyDataReady(value); });
    }

    QObject* _executionContext{ nullptr };
    std::function<void(Request)> _notifyDataReady;
    //    std::function<void(grpc::Status)> _notifyClose;
};

//==============================================================================

struct BaseAsyncClientCall {
    enum class State { Initial, Connecting, MetadataRecv, Connected, Finishing, Finished, Shutdown };

    using GenerateTag = std::function<void*(std::string)>;

    BaseAsyncClientCall(
        State state, std::unique_ptr<ClientContext> context, GenerateTag generateTag)
        : _state(state)
        , _context(std::move(context))
        , _generateTag(generateTag)
    {
		
    }

    virtual ~BaseAsyncClientCall() {}

    State _state{ State::Initial };

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    std::unique_ptr<ClientContext> _context;

    // Storage for the status of the RPC upon completion.
    Status _status;

    GenerateTag _generateTag;

    grpc::Alarm _alarm;

    virtual void process(bool ok, std::string payload) = 0;
};

//==============================================================================

// struct for keeping state and data information
template <class T> struct UnaryAsyncCall : public BaseAsyncClientCall {
public:
    UnaryAsyncCall(std::unique_ptr<ClientContext> context, GenerateTag generateTag)
        : BaseAsyncClientCall(State::Initial, std::move(context), generateTag)
    {
    }

    template <class F, class Request>
    void makeRequest(F&& func, Request&& request, CompletionQueue* cq, 
		const QtPromise::QPromiseResolve<T>& resolve,
		const QtPromise::QPromiseReject<T>& reject)
    {
        _reader = func(_context.get(), request, cq);
		_resolve = resolve;
		_reject = reject;
    }

protected:
    void process(bool ok, std::string /*payload*/) override
    {
        if (_state == State::Initial) {
            _reader->StartCall();
            _reader->Finish(&_reply, &_status, _generateTag({}));
            _state = State::Connecting;
        } else if (ok && _status.ok()) {
            _resolve.get()(_reply);
            _state = State::Finished;
        } else if (_state == State::Shutdown) {
            if (_reject) {
                _reject.get()(Status::CANCELLED);
            }
        } else {
            _reject.get()(_status);
            _state = State::Finished;
        }
    }

private:
    // Container for the data we expect from the server.
    T _reply;
    std::unique_ptr<ClientAsyncResponseReader<T>> _reader;
    boost::optional<QtPromise::QPromiseResolve<T>> _resolve;
    boost::optional<QtPromise::QPromiseReject<T>> _reject;
};

//==============================================================================

template <class T> struct StreamingReadAsyncCall : public BaseAsyncClientCall {

public:
    using Observer = StreamObserver<T>;
    using ObserverUniqueRef = std::unique_ptr<Observer>;
    StreamingReadAsyncCall(std::unique_ptr<ClientContext> context, GenerateTag generateTag)
        : BaseAsyncClientCall(State::Initial, std::move(context), generateTag)
    {
		LogCDebug(GRPC) << "Creating StreamingReadAsyncCall" << reinterpret_cast<uintptr_t>(this);
    }
	~StreamingReadAsyncCall()
	{
		LogCDebug(GRPC) << "Destryoing StreamingReadAsyncCall" << reinterpret_cast<uintptr_t>(this);
	}

    template <class F, class Request>
    void makeRequest(
        F&& func, Request&& request, ObserverUniqueRef readContext, CompletionQueue* cq)
    {
        _observer = std::move(readContext);
        _reader = func(_context.get(), request, cq);
    }

    void process(bool ok, std::string /*payload*/) override
    {
		LogCDebug(GRPC) << "Processing call:" << reinterpret_cast<uintptr_t>(this)
			<< "state:" << static_cast<int>(_state) << "isOk:" << ok;

        if (_state == State::Shutdown || _state == State::Finished) {
            return;
        }

        if (_state == State::Finishing) {
            _state = State::Finished;
            _observer->onCompleted(_status);
			return;
        }

		if (ok) {
			if (_state == State::Initial) {
				_state = State::Connecting;
				_reader->StartCall(_generateTag({}));
			}
			else if (_state == State::Connected) {
				// we can get here only if we are on read or write
				notifyRead();
			}
			else if (_state == State::Connecting) {
				_state = State::MetadataRecv;
				_reader->ReadInitialMetadata(_generateTag({}));
			}
			else if (_state == State::MetadataRecv) {
				_state = State::Connected;
				read(); // start reading right away
			}
		} else {
			if (_state == State::Initial) {
				LogCDebug(GRPC) << "Grpc canceled:" << reinterpret_cast<uintptr_t>(this);
				_state = State::Finished;
				_observer->onCompleted(grpc::Status::CANCELLED);
			} else {
				_state = State::Finishing;
				_reader->Finish(&_status, _generateTag({}));
			}
		}
    }

private:
    void read()
    {
        Q_ASSERT(!_readPending);
		_reader->Read(&_reply, _generateTag({}));
        _readPending = true;
    }

    void notifyRead()
    {
        if (_readPending) {
            _readPending = false;

            auto reply = std::move(_reply);
            _observer->onNext(reply);

            read();
        }
    }

private:
    // Container for the data we expect from the server.
    T _reply;
    std::unique_ptr<ClientAsyncReader<T>> _reader;
    ObserverUniqueRef _observer;
    bool _readPending{ false };
};

//==============================================================================

template <class Request, class Response>
struct StreamingReadWriteAsyncCall : public BaseAsyncClientCall {

public:
    using Observer = StreamRequestObserver<Request, Response>;
    using ObserverUniqueRef = std::unique_ptr<Observer>;

    StreamingReadWriteAsyncCall(
        std::unique_ptr<ClientContext> context, GenerateTag generateTag, QObject* executionContext)
        : BaseAsyncClientCall(State::Initial, std::move(context), generateTag)
        , _executionContext(executionContext)
    {
    }

    template <class F>
    void makeRequest(F&& func, ObserverUniqueRef clientContext, CompletionQueue* cq)
    {
        subscribeForEvents(clientContext);
        _readerWriter = func(_context.get(), cq);
    }

    void process(bool ok, std::string payload) override
    {
        LogCDebug(GRPC) << "Processing call:" << reinterpret_cast<uintptr_t>(this)
                        << "state:" << static_cast<int>(_state) << "isOk:" << ok;

        if (_state == State::Shutdown || _state == State::Finished) {
            return;
        }

        if (_state == State::Finishing) {
            _state = State::Finished;
            _eventSubscription->onCompleted(_status);
            return;
        }

        if (ok) {
            if (_state == State::Initial) {
                _readerWriter->StartCall(_generateTag({}));
                _state = State::Connecting;
            } else if (_state == State::Connected) {
                if (payload == _readPayload) {
                    // we can get here only if we are on read
                    onReadDone();
                } else if (payload == _writePayload) {
                    onWriteDone();
                }
            } else if (_state == State::Connecting) {
                _state = State::Connected;
                read(); // start reading right away
                write();
            }
        } else {
            _state = State::Finishing;
            _readerWriter->Finish(&_status, _generateTag({}));
        }
    }

    void subscribeForEvents(ObserverUniqueRef eventHandler)
    {
        LogCDebug(GRPC) << "Subscribing for events" << eventHandler.get();
        _eventSubscription = std::move(eventHandler);
        _eventSubscription->_executionContext = _executionContext;
        _eventSubscription->_notifyDataReady = [this](Request data) {
            Q_ASSERT(_executionContext->thread() == QThread::currentThread());
            _writeQueue.push(data);
            write();
        };
    }

private:
    void read()
    {
        Q_ASSERT(!_readPending);
        _readerWriter->Read(&_reply, _generateTag(_readPayload));
        _readPending = true;
    }

    void write()
    {
        if (_writePending || _writeQueue.empty() || _state != State::Connected) {
            return;
        }

        auto item = _writeQueue.front();
        _writeQueue.pop();
        _writePending = true;

        _readerWriter->Write(item, _generateTag(_writePayload));
    }

    void onReadDone()
    {
        if (_readPending) {
            _readPending = false;
            auto reply = std::move(_reply);
            _eventSubscription->onNext(reply);

            read();
        }
    }

    void onWriteDone()
    {
        if (_writePending) {
            _writePending = false;
            write();
        }
    }

private:
    // Container for the data we expect from the server.
    QObject* _executionContext;
    Response _reply;
    std::queue<Request> _writeQueue;
    std::unique_ptr<ClientAsyncReaderWriter<Request, Response>> _readerWriter;
    ObserverUniqueRef _eventSubscription;
    bool _readPending{ false };
    bool _writePending{ false };
    const std::string _readPayload{ "r" };
    const std::string _writePayload{ "w" };
};

//==============================================================================

class BaseGrpcClient : public QObject {

    Q_OBJECT

    struct CallOps {
        uint64_t id;
        std::string payload;
    };

public:
    using TlsCertProvider = std::function<std::string()>;
    using MacaroonProvider = std::function<std::string()>;
    enum class AuthType {
        SSL,
        Macaroon,
        None,
    };

    static const grpc::Status notConnectedStatus;

    explicit BaseGrpcClient(QString rpcChannel, TlsCertProvider tlsCertProvider,
        MacaroonProvider macaroonProvider, AuthType authType = AuthType::Macaroon,
        QObject* parent = nullptr);
    ~BaseGrpcClient();

    template <class T, class F, class Request, class Client>
    Promise<T> makeUnaryRequest(Client* client, F&& func, Request&& req, uint32_t timeout)
    {
		return Promise<T>([=](const auto &resolve, const auto &reject) {
			QMetaObject::invokeMethod(_worker.context(), [=] {
				using CallType = UnaryAsyncCall<T>;
				auto id = this->generateId();
				auto generateTag
					= [this, id](std::string payload) -> void* { return this->generateCallOps(id, payload); };
				std::unique_ptr<BaseAsyncClientCall> call(
					new CallType(CreateContext(timeout), generateTag));
				static_cast<CallType*>(call.get())
					->makeRequest(std::bind(func, client, std::placeholders::_1,
						std::placeholders::_2, std::placeholders::_3),
						req, &_cq, resolve, reject);

				this->startPreparedCall(id, std::move(call));
			});
		});
    }

    template <class T, class F, class Request, class Client>
    void makeStreamingReadRequest(Client* client, F&& func, Request&& req,
        std::unique_ptr<StreamObserver<T>> readContext, uint32_t timeout)
    {
		QMetaObject::invokeMethod(_worker.context(), [this, client, func, req, timeout, readContext = std::move(readContext)]() mutable {
			using CallType = StreamingReadAsyncCall<T>;
			auto id = this->generateId();
			auto generateTag
				= [this, id](std::string payload) -> void* { return this->generateCallOps(id, payload); };

			std::unique_ptr<BaseAsyncClientCall> call(
				new CallType(CreateContext(timeout), generateTag));

			LogCDebug(GRPC) << "Created streaming call" << reinterpret_cast<uintptr_t>(call.get());

			static_cast<CallType*>(call.get())
				->makeRequest(std::bind(func, client, std::placeholders::_1, std::placeholders::_2,
					std::placeholders::_3),
					req, std::move(readContext), &_cq);

			this->startPreparedCall(id, std::move(call));
		});
    }

    template <class T, class F, class Request, class Client>
    void makeStreamingReadWriteRequest(Client* client, F&& func,
        std::unique_ptr<StreamRequestObserver<Request, T>> readWriteContext, uint32_t timeout)
    {
		QMetaObject::invokeMethod(_worker.context(), [this, client, func, timeout, readWriteContext = std::move(readWriteContext)]() mutable {
			using CallType = StreamingReadWriteAsyncCall<Request, T>;
			auto id = this->generateId();
			auto generateTag
				= [this, id](std::string payload) -> void* { return this->generateCallOps(id, payload); };
			std::unique_ptr<BaseAsyncClientCall> call(
				new CallType(CreateContext(timeout), generateTag, _worker.context()));

			LogCDebug(GRPC) << "Created streaming call" << reinterpret_cast<uintptr_t>(call.get());

			static_cast<CallType*>(call.get())
				->makeRequest(std::bind(func, client, std::placeholders::_1, std::placeholders::_2),
					std::move(readWriteContext), &_cq);

			this->startPreparedCall(id, std::move(call));
		});
    }

    static std::unique_ptr<ClientContext> CreateContext(size_t timeoutMs = 2500);

    void tearDown();
    bool init();
    void connect();

private:
    uint64_t generateId();
    CallOps* generateCallOps(uint64_t id, std::string payload);
    void completeCall(uint64_t id);
    void completeAllCalls();
    BaseAsyncClientCall* pendingCall(uint64_t id);
    void completeRpc();
    void startWorker();
    void startPreparedCall(uint64_t id, std::unique_ptr<BaseAsyncClientCall> preparedCall);

protected:
    std::shared_ptr<grpc::Channel> _channel;

private:
    QSemaphore _semaphore;
    QString _rpcChannel;
    TlsCertProvider _tlsCertProvider;
    MacaroonProvider _macaroonProvider;
    Utils::WorkerThread _worker;
    bool _isShutdown{ false };
    uint64_t _lastUsedId{ 0 };
    AuthType _authType{ AuthType::SSL };

    // The producer-consumer queue we use to communicate asynchronously with the
    // gRPC runtime.
    CompletionQueue _cq;

    std::unordered_map<uint64_t, std::unique_ptr<BaseAsyncClientCall>> _pendingCalls;
};

#endif // CLIENTUTILS_HPP
