#include "Web3Client.hpp"

#include <Utils/Logging.hpp>

using namespace jcon;

//==============================================================================

Web3Client::Web3Client(QUrl host, QObject* parent)
    : AbstractWeb3Client(parent)
    , _host(host)
    , _client(new JsonRpcWebSocketClient(this, nullptr, 20000))
{
    init();
}

//==============================================================================

void Web3Client::open()
{
    _autoConnectTimer->start(0);
}

//==============================================================================

auto Web3Client::getBalance(QString address) -> Promise<eth::u256>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(
            this, [=] { callAsync(resolve, reject, "eth_getBalance", address, "latest"); });
    })
        .then([](QVariant result) { return eth::u256{ result.toString().toStdString() }; });
}

//==============================================================================

auto Web3Client::getBlockNumber() -> Promise<eth::u64>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] { callAsync(resolve, reject, "eth_blockNumber"); });
    })
        .then([](QVariant result) { return eth::u64{ result.toString().toStdString() }; });
}

//==============================================================================

auto Web3Client::getBlockByNumber(QString blockNumber) -> Promise<QVariantMap>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(
            this, [=] { callAsync(resolve, reject, "eth_getBlockByNumber", blockNumber, false); });
    })
        .then([](QVariant result) { return result.toMap(); });
}

//==============================================================================

auto Web3Client::call(QVariantMap params) -> Promise<QString>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(
            this, [=] { callAsync(resolve, reject, "eth_call", params, "latest"); });
    })
        .then([](QVariant result) { return result.toString(); });
}

//==============================================================================

auto Web3Client::getChainId() -> Promise<eth::u256>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] { callAsync(resolve, reject, "eth_chainId"); });
    })
        .then([](QVariant result) { return eth::u256{ result.toString().toStdString() }; });
}

//==============================================================================

auto Web3Client::estimateGas(QVariantMap params) -> Promise<eth::u64>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(
            this, [=] { callAsync(resolve, reject, "eth_estimateGas", params); });
    })
        .then([](QVariant result) { return eth::u64{ result.toString().toStdString() }; });
}

//==============================================================================

auto Web3Client::getTransactionCount(QString addressTo) -> Promise<eth::u64>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this,
            [=] { callAsync(resolve, reject, "eth_getTransactionCount", addressTo, "latest"); });
    })
        .then([](QVariant result) { return eth::u64{ result.toString().toStdString() }; });
}

//==============================================================================

auto Web3Client::getTransactionByHash(QString txHash) -> Promise<QVariantMap>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(
            this, [=] { callAsync(resolve, reject, "eth_getTransactionByHash", txHash); });
    })
        .then([](QVariant result) { return result.toMap(); });
}

//==============================================================================

auto Web3Client::getTransactionReceipt(QString txHash) -> Promise<QVariantMap>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(
            this, [=] { callAsync(resolve, reject, "eth_getTransactionReceipt", txHash); });
    })
        .then([](QVariant result) { return result.toMap(); });
}

//==============================================================================

auto Web3Client::sendRawTransaction(QString serializedTxHex) -> Promise<QString>
{
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(
            this, [=] { callAsync(resolve, reject, "eth_sendRawTransaction", serializedTxHex); });
    })
        .then([](QVariant result) {
            qDebug() << "callAsync sendRawTransaction";
            return result.toString();
        });
}

//==============================================================================

auto Web3Client::subscribe(QString eventType) -> Promise<void>
{
    //    _client->notification("eth_subscribe",
    //    eventType);
    return Promise<QVariant>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(
            this, [=] { callAsync(resolve, reject, "eth_subscribe", eventType); });
    })
        .then([](QVariant result) { qDebug() << "callAsync subscribe"; });

    //    QtPromise::resolve();
}

//==============================================================================

void Web3Client::onTryAutoConnect()
{
    LogCDebug(Web3) << "Connecting to web3 host";
    _client->connectToServer(_host);
}

//==============================================================================

void Web3Client::onConnected()
{
    LogCDebug(Web3) << "Web3 client connected";
    _autoConnectTimer->stop();
    connected();
}

//==============================================================================

void Web3Client::onDisconnected()
{
    LogCDebug(Web3) << "Web3 client disconnected";
    maybeStartReconnecting();
}

//==============================================================================

void Web3Client::onConnectionError(QAbstractSocket::SocketError error)
{
    LogCCritical(Web3) << "Web3 connection error:" << error;
    maybeStartReconnecting();
}

//==============================================================================

void Web3Client::init()
{
    qRegisterMetaType<QAbstractSocket::SocketState>("QAbstractSocket::SocketState");
    LogCDebug(Web3) << "Creating web3 "
                       "connection to host:"
                    << _host;
    connect(_client, &JsonRpcWebSocketClient::socketConnected, this, &Web3Client::onConnected);
    connect(
        _client, &JsonRpcWebSocketClient::socketDisconnected, this, &Web3Client::onDisconnected);
    connect(_client, &JsonRpcWebSocketClient::socketError, this,
        std::bind(&Web3Client::onConnectionError, this, std::placeholders::_2));

    _autoConnectTimer = new QTimer(this);
    _autoConnectTimer->setSingleShot(true);
    connect(_autoConnectTimer, &QTimer::timeout, this, &Web3Client::onTryAutoConnect);

    _client->enableReceiveNotification(true);

    connect(_client, &JsonRpcWebSocketClient::notificationReceived, this,
        [](const QString key, const QVariant value) {
            qDebug() << "notificationReceived" << key << value;
        });
}

//==============================================================================

void Web3Client::maybeStartReconnecting()
{
    if (!_closed) {
        _autoConnectTimer->start(5000);
    }
}

//==============================================================================
