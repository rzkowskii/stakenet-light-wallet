#ifndef LNDGRPCCLIENT_HPP
#define LNDGRPCCLIENT_HPP

#include <GRPCTools/ClientUtils.hpp>
#include <LndTools/Protos/autopilot.grpc.pb.h>
#include <LndTools/Protos/invoices.grpc.pb.h>
#include <LndTools/Protos/router.grpc.pb.h>
#include <LndTools/Protos/rpc.grpc.pb.h>
#include <LndTools/Protos/watchtower.grpc.pb.h>
#include <LndTools/Protos/wtclient.grpc.pb.h>
#include <QObject>
#include <QTimer>
#include <grpcpp/grpcpp.h>

class QTimer;

class LndGrpcClient : public BaseGrpcClient {
    Q_OBJECT
public:
    template <class T> using StreamingReadObserver = std::unique_ptr<StreamObserver<T>>;
    template <class Request, class Response>
    using StreamingReadWriteObserver = std::unique_ptr<StreamRequestObserver<Request, Response>>;
    explicit LndGrpcClient(QString rpcChannel, TlsCertProvider tlsCertProvider,
        MacaroonProvider macaroonProvider = {}, QObject* parent = nullptr);
    ~LndGrpcClient();

    void connect();
    bool isConnected() const;
    void close();

    template <class T, class F, class Request>
    Promise<T> makeRpcUnaryRequest(F&& func, Request&& req, uint32_t timeout = 2500)
    {
        if (!isConnected()) {
            return Promise<T>::reject(notConnectedStatus);
        }

        return makeUnaryRequest<T>(_rpcClient.get(), func, req, timeout);
    }

    template <class T, class F, class Request>
    Promise<T> makeWatchTowerUnaryRequest(F&& func, Request&& req, uint32_t timeout = 2500)
    {
        if (!isConnected()) {
            return Promise<T>::reject(notConnectedStatus);
        }

        return makeUnaryRequest<T>(_watchTowerClient.get(), func, req, timeout);
    }

    template <class T, class F, class Request>
    Promise<T> makeInvoicesUnaryRequest(F&& func, Request&& req, uint32_t timeout = 2500)
    {
        if (!isConnected()) {
            return Promise<T>::reject(notConnectedStatus);
        }

        return makeUnaryRequest<T>(_invoicesClient.get(), func, req, timeout);
    }

    template <class T, class F, class Request>
    void makeInvoicesStreamingRequest(
        F&& func, Request&& req, StreamingReadObserver<T> readContext, uint32_t timeout = 2500)
    {
        makeStreamingReadRequest<T>(
            _invoicesClient.get(), func, req, std::move(readContext), timeout);
    }

    template <class T, class F, class Request>
    void makeRpcStreamingRequest(
        F&& func, Request&& req, StreamingReadObserver<T> readContext, uint32_t timeout = 2500)
    {
        makeStreamingReadRequest<T>(_rpcClient.get(), func, req, std::move(readContext), timeout);
    }

    template <class T, class F, class Request>
    void makeRpcBidiStreamingRequest(
        F&& func, StreamingReadWriteObserver<Request, T> readWriteContext, uint32_t timeout = 2500)
    {
        makeStreamingReadWriteRequest(_rpcClient.get(), func, std::move(readWriteContext), timeout);
    }

    template <class T, class F, class Request>
    void makeRouterStreamingRequest(
        F&& func, Request&& req, StreamingReadObserver<T> readContext, uint32_t timeout = 2500)
    {
        makeStreamingReadRequest<T>(
            _routerClient.get(), func, req, std::move(readContext), timeout);
    }

    template <class T, class F, class Request>
    Promise<T> makeAutopilotUnaryRequest(F&& func, Request&& req, uint32_t timeout = 2500)
    {
        if (!isConnected()) {
            return Promise<T>::reject(notConnectedStatus);
        }

        return makeUnaryRequest<T>(_autopilotClient.get(), func, req, timeout);
    }

signals:
    void connected();

private:
    void tryConnect();

private:
    QTimer* _connectionTimer{ nullptr };
    std::unique_ptr<lnrpc::Lightning::Stub> _rpcClient;
    std::unique_ptr<invoicesrpc::Invoices::Stub> _invoicesClient;
    std::unique_ptr<autopilotrpc::Autopilot::Stub> _autopilotClient;
    std::unique_ptr<routerrpc::Router::Stub> _routerClient;
    std::unique_ptr<wtclientrpc::WatchtowerClient::Stub> _watchTowerClient;
};
#endif // LNDGRPCCLIENT_HPP
