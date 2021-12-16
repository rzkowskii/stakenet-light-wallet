#ifndef WEB3CLIENT_HPP
#define WEB3CLIENT_HPP

#include <Networking/AbstractWeb3Client.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <QObject>
#include <QPointer>
#include <QUrl>
#include <Utils/Logging.hpp>
#include <json_rpc_websocket_client.h>

class QTimer;

class Web3Client : public AbstractWeb3Client {
    Q_OBJECT
public:
    explicit Web3Client(QUrl host, QObject* parent = nullptr);

    void open() override;

    Promise<eth::u256> getBalance(QString address) override;
    Promise<eth::u64> getBlockNumber() override;
    Promise<QVariantMap> getBlockByNumber(QString blockNumber) override;
    Promise<QString> call(QVariantMap params) override;
    Promise<eth::u256> getChainId() override;
    Promise<eth::u64> estimateGas(QVariantMap params) override;
    Promise<eth::u64> getTransactionCount(QString addressTo) override;
    Promise<QVariantMap> getTransactionByHash(QString txHash) override;
    Promise<QVariantMap> getTransactionReceipt(QString txHash) override;

    Promise<QString> sendRawTransaction(QString serializedTxHex) override;

    // TODO: subscribe is broken, need to make changes in lib for proper using
    Promise<void> subscribe(QString eventType) override;

private slots:
    void onTryAutoConnect();
    void onConnected();
    void onDisconnected();
    void onConnectionError(QAbstractSocket::SocketError error);

private:
    void init();
    void maybeStartReconnecting();

    template <typename... T>
    void callAsync(QtPromise::QPromiseResolve<QVariant> resolve,
        QtPromise::QPromiseReject<QVariant> reject, const QString& method, T&&... args)
    {
        auto call = _client->callAsync(method, std::forward<T>(args)...);
        connect(call.get(), &jcon::JsonRpcRequest::error, call.get(),
            [reject](int code, const QString& message, const QVariant& data) {
                LogCCritical(Web3) << "Web3 call failed: " << code << message << data;
                reject(NetworkUtils::ApiErrorException{ code, message });
            });
        connect(call.get(), &jcon::JsonRpcRequest::result, call.get(),
            [resolve](QVariant res) { resolve(res); });
    }

private:
    QUrl _host;
    QPointer<jcon::JsonRpcWebSocketClient> _client;
    QPointer<QTimer> _autoConnectTimer;
    bool _closed{ false };
};

#endif // WEB3CLIENT_HPP
