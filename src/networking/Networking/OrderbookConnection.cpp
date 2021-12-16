#include "OrderbookConnection.hpp"
#include <Networking/NetworkConnectionState.hpp>
#include <Utils/Logging.hpp>

#include <QTimer>

namespace orderbook {

namespace OrderbookParamsKeys {
    const QString HEADER_API_VERSION{ "Client-Version" };
    const QString HEADER_BOT_MAKER_SECRET{ "BotMaker-Secret" };
    const QString HEADER_CLIENT_ID{ "Light-Wallet-Unique-Id" };
}

//==============================================================================

static QNetworkRequest BuildOrderbookRequest(QUrl host, std::map<QString, QString> params)
{
    using namespace OrderbookParamsKeys;
    static const QString allowedHeaders[] = {
        HEADER_API_VERSION,
        HEADER_BOT_MAKER_SECRET,
        HEADER_CLIENT_ID,
    };

    QNetworkRequest request;
    request.setUrl(host);

    for (auto&& header : allowedHeaders) {
        if (params.count(header) > 0) {
            auto value = params.at(header);
            if (!value.isEmpty()) {
                request.setRawHeader(header.toLatin1(), value.toLatin1());
            } else {
                LogCWarning(Orderbook) << "Skipping header" << header << "because value is empty";
            }
        }
    }

    return request;
}

//==============================================================================

OrderbookConnection::OrderbookConnection(QUrl host, NetworkConnectionState* connectionState,
    std::map<QString, QString> params, QObject* parent)
    : QObject(parent)
    , _request(BuildOrderbookRequest(host, params))
{
    if (connectionState) {
        connect(
            connectionState, &NetworkConnectionState::stateChanged, this, [this](auto newState) {
                if (newState == NetworkConnectionState::State::Disconnected) {
                    _socket->close(QWebSocketProtocol::CloseCodeAbnormalDisconnection);
                } else {
                    this->onTryAutoConnect();
                }
            });
    }
}

//==============================================================================

OrderbookConnection::~OrderbookConnection()
{
    close();
}

//==============================================================================

void OrderbookConnection::open()
{
    init();
    _autoConnectTimer->start(0);
}

//==============================================================================

void OrderbookConnection::reconnect()
{
    _socket->close();
}

//==============================================================================

void OrderbookConnection::close()
{
    _closed = true;
    _socket->close();
}

//==============================================================================

bool OrderbookConnection::isConnected() const
{
    return _socket->state() == QAbstractSocket::ConnectedState;
}

//==============================================================================

bool OrderbookConnection::sendCommand(std::vector<unsigned char> serialized)
{
    LogCDebug(Orderbook) << "Sending orderbook message" << serialized.size();
    return _socket->sendBinaryMessage(QByteArray::fromRawData(
               reinterpret_cast<char*>(&serialized[0]), static_cast<int>(serialized.size())))
        == static_cast<int>(serialized.size());
}

//==============================================================================

void OrderbookConnection::onTryAutoConnect()
{
    _socket->open(_request);
}

//==============================================================================

void OrderbookConnection::onConnected()
{
    LogCDebug(Orderbook) << "Orderbook client connected";
    _autoConnectTimer->stop();
    connected();
}

//==============================================================================

void OrderbookConnection::onDisconnected()
{
    LogCDebug(Orderbook) << "Orderbook client disconnected" << _socket->closeCode()
                         << _socket->closeReason();
    maybeStartReconnecting();
    disconnected();
}

//==============================================================================

void OrderbookConnection::onConnectionError(QAbstractSocket::SocketError error)
{
    LogCCritical(Orderbook) << "Orderbook connection error:" << error << _socket->errorString();
    maybeStartReconnecting();
    connectionError(error);
}

//==============================================================================

void OrderbookConnection::onMessageReceived(QByteArray message)
{
    payloadReceived(message);
}

//==============================================================================

void OrderbookConnection::init()
{
    if (_socket) {
        return;
    }

    _socket = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
    connect(_socket, &QWebSocket::connected, this, &OrderbookConnection::onConnected);
    connect(_socket, &QWebSocket::disconnected, this, &OrderbookConnection::onDisconnected);
    connect(_socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this,
        &OrderbookConnection::onConnectionError);
    connect(
        _socket, &QWebSocket::binaryMessageReceived, this, &OrderbookConnection::onMessageReceived);

    _autoConnectTimer = new QTimer(this);
    _autoConnectTimer->setSingleShot(true);
    connect(_autoConnectTimer, &QTimer::timeout, this, &OrderbookConnection::onTryAutoConnect);
}

//==============================================================================

void OrderbookConnection::maybeStartReconnecting()
{
    if (!_closed) {
        _autoConnectTimer->start(5000);
    }
}

//==============================================================================
}
