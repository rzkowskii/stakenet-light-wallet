#ifndef SERVERUTILS_HPP
#define SERVERUTILS_HPP

#include <Utils/Logging.hpp>

#include <QCoreApplication>
#include <QObject>
#include <QThread>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <type_traits>
#include <unordered_map>

#include <boost/function_types/parameter_types.hpp>
#include <boost/mpl/at.hpp>
#include <boost/optional.hpp>
#include <grpc++/alarm.h>
#include <grpc/support/log.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/impl/codegen/async_stream.h>
#include <grpcpp/impl/codegen/async_unary_call.h>

namespace qgrpc {
namespace details {

    template <class> struct IsStreamingCall : std::false_type {
    };

    template <class T> struct IsStreamingCall<grpc::ServerAsyncWriter<T>> : std::true_type {
        typedef T type;
    };

    template <class> struct IsUnaryCall : std::false_type {
    };

    template <class T> struct IsUnaryCall<grpc::ServerAsyncResponseWriter<T>> : std::true_type {
        typedef T type;
    };

    template <typename T> constexpr bool is_streaming_call_v = IsStreamingCall<T>::value;

    template <typename T> constexpr bool is_unary_call_v = IsUnaryCall<T>::value;

    template <class Responder, typename Enable = void> struct RequestTypeResolver;

    template <class Responder>
    struct RequestTypeResolver<Responder,
        typename std::enable_if<is_streaming_call_v<Responder>>::type> {
        typedef typename IsStreamingCall<Responder>::type raw_type;
        typedef
            typename std::remove_const<typename std::remove_reference<raw_type>::type>::type type;
    };

    template <class Responder>
    struct RequestTypeResolver<Responder,
        typename std::enable_if<is_unary_call_v<Responder>>::type> {
        typedef typename IsUnaryCall<Responder>::type raw_type;
        typedef
            typename std::remove_const<typename std::remove_reference<raw_type>::type>::type type;
    };

    template <typename F> struct call_types {
        typedef typename boost::function_types::parameter_types<F>::type function_types;
        typedef typename std::remove_pointer<
            typename boost::mpl::at<function_types, boost::mpl::int_<2>>::type>::type Request;
        typedef typename std::remove_pointer<
            typename boost::mpl::at<function_types, boost::mpl::int_<3>>::type>::type Responder;
        typedef typename RequestTypeResolver<Responder>::type Reply;
    };

    template <typename F>
    constexpr bool is_streaming = is_streaming_call_v<typename call_types<F>::Responder>;

    template <typename F>
    constexpr bool is_unary = is_unary_call_v<typename call_types<F>::Responder>;
}

//==============================================================================

template <class T> struct StreamingChannel : std::enable_shared_from_this<StreamingChannel<T>> {
    explicit StreamingChannel(QObject* context)
        : _context(context)
    {
    }

    void send(T value)
    {
        QMetaObject::invokeMethod(_context,
            [=] {
                _writeQueue.push(value);
                _notifyDataReady();
            },
            Qt::QueuedConnection);
    }

    void close(grpc::Status status)
    {
        QMetaObject::invokeMethod(_context, [=] { _notifyClose(status); });
    }

    QObject* _context;
    std::queue<T> _writeQueue;
    std::function<void(void)> _notifyDataReady;
    std::function<void(grpc::Status)> _notifyClose;
};

//==============================================================================

template <class T> struct UnarySender : std::enable_shared_from_this<UnarySender<T>> {
    explicit UnarySender(QObject* context)
        : _context(context)
    {
    }

    void finish(T value, grpc::Status status = grpc::Status::OK)
    {
        QMetaObject::invokeMethod(_context, [=] { _notifyReplyReady(status, value); });
    }

    void finish(grpc::Status status)
    {
        QMetaObject::invokeMethod(_context, [=] { _notifyReplyReady(status, boost::none); });
    }

    QObject* _context;
    std::function<void(grpc::Status, boost::optional<T>)> _notifyReplyReady;
};

//==============================================================================

struct BaseServerAsyncCall {
    enum class State { Create, Connecting, Processing, Finishing, Finished };

    BaseServerAsyncCall(uint64_t id, grpc::ServerCompletionQueue* cq, QObject* context)
        : _executionContext(context)
        , _id(id)
        , _cq(cq)
    {
    }

    void setState(State newState) { _state = newState; }

    virtual ~BaseServerAsyncCall() {}

    virtual void process(bool ok) = 0;
    virtual std::unique_ptr<BaseServerAsyncCall> replicateAndProcess(uint64_t newId) = 0;

    QObject* _executionContext{ nullptr };
    uint64_t _id;
    grpc::ServerCompletionQueue* _cq;
    grpc::ServerContext _ctx;
    State _state{ State::Create }; // The current serving state.
};

//==============================================================================

template <typename F, typename Service> struct UnaryServerAsyncCall : BaseServerAsyncCall {
    typedef typename details::call_types<F>::Request Request;
    typedef typename details::call_types<F>::Reply Reply;
    typedef typename details::call_types<F>::Responder Responder;

    using Receiver
        = std::function<void(grpc::ServerContext*, Request*, std::shared_ptr<UnarySender<Reply>>)>;

    explicit UnaryServerAsyncCall(uint64_t id, Service* service, grpc::ServerCompletionQueue* cq,
        F func, Receiver receiver, QObject* context)
        : BaseServerAsyncCall(id, cq, context)
        , _func(func)
        , _receiver(receiver)
        , _responder(&_ctx)
        , _service(service)
    {
    }

    std::unique_ptr<BaseServerAsyncCall> replicateAndProcess(uint64_t newId) override
    {
        auto result = std::unique_ptr<BaseServerAsyncCall>(
            new UnaryServerAsyncCall(newId, _service, _cq, _func, _receiver, _executionContext));
        setState(State::Processing);
        return result;
    }

    void process(bool ok) override
    {
        Q_ASSERT_X(QThread::currentThread() == _executionContext->thread(), "Execution thread",
            "Missmatched thread affinity");
        if (ok) {
            if (_state == State::Create) {
                std::bind(_func, _service, &_ctx, &_request, &_responder, _cq, _cq,
                    reinterpret_cast<void*>(_id))();
                setState(State::Connecting);
            } else if (_state == State::Processing) {
                _sender = std::make_shared<UnarySender<Reply>>(_executionContext);
                _sender->_notifyReplyReady
                    = [this](grpc::Status status, boost::optional<Reply> reply) {
                          // if we are waiting for write to complete - do nothing since data
                          // will get eventually written back to client
                          auto tag = reinterpret_cast<void*>(_id);
                          if (reply) {
                              _responder.Finish(reply.get(), status, tag);
                          } else {
                              _responder.FinishWithError(status, tag);
                          }

                          setState(State::Finishing);
                      };

                _receiver(&_ctx, &_request, _sender);
            } else if (_state == State::Finishing) {
                setState(State::Finished);
            }
        } else {
            if (_sender) {
                _sender->_notifyReplyReady = [](auto, auto) {};
            }

            if (_state == State::Processing) {
                auto tag = reinterpret_cast<void*>(_id);
                _responder.FinishWithError(
                    grpc::Status(grpc::StatusCode::CANCELLED, "Canceled by shutdown"), tag);
            }
            setState(State::Finished);
        }
    }

    F _func;
    Request _request;
    Receiver _receiver;
    std::shared_ptr<UnarySender<Reply>> _sender;
    Responder _responder;
    Service* _service{ nullptr };
};

//==============================================================================

template <typename F, typename Service> struct StreamingServerAsyncCall : BaseServerAsyncCall {
public:
    typedef typename details::call_types<F>::Request Request;
    typedef typename details::call_types<F>::Reply Reply;
    typedef typename details::call_types<F>::Responder Responder;

    using Receiver = std::function<void(
        grpc::ServerContext*, Request*, std::shared_ptr<StreamingChannel<Reply>>)>;

    explicit StreamingServerAsyncCall(uint64_t id, Service* service,
        grpc::ServerCompletionQueue* cq, F func, Receiver receiver, QObject* context)
        : BaseServerAsyncCall(id, cq, context)
        , _func(func)
        , _receiver(receiver)
        , _responder(&_ctx)
        , _service(service)
    {
    }

    std::unique_ptr<BaseServerAsyncCall> replicateAndProcess(uint64_t newId) override
    {
        auto result = std::unique_ptr<BaseServerAsyncCall>(new StreamingServerAsyncCall(
            newId, _service, _cq, _func, _receiver, _executionContext));
        setState(State::Processing);
        return result;
    }

    void process(bool ok) override
    {
        Q_ASSERT_X(QThread::currentThread() == _executionContext->thread(), "Execution thread",
            "Missmatched thread affinity");
        if (ok) {
            if (_state == State::Create) {
                std::bind(_func, _service, &_ctx, &_request, &_responder, _cq, _cq,
                    reinterpret_cast<void*>(_id))();
                _ctx.AsyncNotifyWhenDone(reinterpret_cast<void*>(_id));
                setState(State::Connecting);
            } else if (_state == State::Processing) {
                // Now that we go through this stage multiple times,
                // we don't want to create a new instance every time.
                // Refer to gRPC's original example if you don't understand
                // why we create a new instance of CallData here.
                if (!_channel) {
                    _channel = std::make_shared<StreamingChannel<Reply>>(_executionContext);
                    _channel->_notifyDataReady = [this] {
                        // if we are waiting for write to complete - do nothing since data
                        // will get eventually written back to client
                        if (!_hasPendingWrite) {
                            sendScheduledData();
                        }
                    };

                    _channel->_notifyClose = [this](grpc::Status status) {
                        _pendingClose = status;
                        if (!_hasPendingWrite) {
                            _responder.Finish(_pendingClose.get(), reinterpret_cast<void*>(_id));
                            setState(State::Finishing);
                        }
                    };
                    _receiver(&_ctx, &_request, _channel);
                } else if (_state == State::Finishing) {
                    setState(State::Finished);
                } else {
                    if (_hasPendingWrite) {
                        _hasPendingWrite = false;
                        sendScheduledData();
                    } else {
                        // means we got AsyncNotifyWhenDone from rpc call
                        setState(State::Finished);
                    }
                }
            }
        } else {
            if (_channel) {
                _channel->_notifyDataReady = [] {};
                _channel->_notifyClose = [](auto) {};
            }

            if (_state == State::Processing) {
                auto tag = reinterpret_cast<void*>(_id);
                _responder.Finish(
                    grpc::Status(grpc::StatusCode::CANCELLED, "Canceled by shutdown"), tag);
            }
            setState(State::Finished);
        }
    }

private:
    void sendScheduledData()
    {
        Q_ASSERT(_hasPendingWrite == false);
        auto& queue = _channel->_writeQueue;
        if (!queue.empty()) {
            bool writeLast = queue.size() == 1 && _pendingClose;
            auto value = queue.front();
            queue.pop();
            _hasPendingWrite = true;
            auto tag = reinterpret_cast<void*>(_id);
            if (writeLast) {
                _responder.WriteAndFinish(value, {}, _pendingClose.get(), tag);
                setState(State::Finishing);
            } else {
                _responder.Write(value, tag);
            }
        }
    }

private:
    F _func;
    Request _request;
    Receiver _receiver;
    std::shared_ptr<StreamingChannel<Reply>> _channel;
    Responder _responder;
    boost::optional<grpc::Status> _pendingClose;
    Service* _service{ nullptr };
    bool _hasPendingWrite{ false };
};

//==============================================================================

class BaseGrpcServer {
public:
    explicit BaseGrpcServer(std::string serverAddress);
    virtual ~BaseGrpcServer();

    void run(std::vector<grpc::Service*> services);
    void shutdown();

protected:
    virtual void registerService() = 0;

    void exec();

    template <class F, class Service>
    using StreamingCall = qgrpc::StreamingServerAsyncCall<F, Service>;

    template <class F, class Service> using UnaryCall = qgrpc::UnaryServerAsyncCall<F, Service>;

    template <class F, class Service>
    typename std::enable_if<qgrpc::details::is_streaming<F>, void>::type registerCall(
        F func, Service* service, typename StreamingCall<F, Service>::Receiver receiver)
    {
        auto newId = generateId();
        auto call = std::unique_ptr<qgrpc::BaseServerAsyncCall>(
            new StreamingCall<F, Service>(newId, service, _cq.get(), func, receiver, _context));
        call->process(true);
        _calls.emplace(newId, std::move(call));
    }

    template <class F, class Service>
    typename std::enable_if<qgrpc::details::is_unary<F>, void>::type registerCall(
        F func, Service* service, typename UnaryCall<F, Service>::Receiver receiver)
    {

        auto newId = generateId();
        auto call = std::unique_ptr<qgrpc::BaseServerAsyncCall>(
            new UnaryCall<F, Service>(newId, service, _cq.get(), func, receiver, _context));
        call->process(true);
        _calls.emplace(newId, std::move(call));
    }

private:
    uint64_t generateId();

    QObject* _context{ nullptr };
    std::string _serverAddress;
    std::unique_ptr<grpc::ServerCompletionQueue> _cq;
    std::unique_ptr<grpc::Server> _server;
    std::unordered_map<uint64_t, std::unique_ptr<qgrpc::BaseServerAsyncCall>> _calls;
    bool _isShutdown{ false };
};
}

#endif // SERVERUTILS_HPP
