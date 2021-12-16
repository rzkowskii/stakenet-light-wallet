#ifndef ORDERBOOKCONNECTION_HPP
#define ORDERBOOKCONNECTION_HPP

#include <QObject>
#include <QPointer>
#include <QWebSocket>
#include <map>

class QTimer;
class NetworkConnectionState;

namespace orderbook {

namespace OrderbookParamsKeys {
    extern const QString HEADER_API_VERSION;
    extern const QString HEADER_BOT_MAKER_SECRET;
    extern const QString HEADER_CLIENT_ID;
}

class OrderbookConnection : public QObject {
    Q_OBJECT
public:
    explicit OrderbookConnection(QUrl host, NetworkConnectionState* connectionState,
        std::map<QString, QString> params, QObject* parent = nullptr);
    ~OrderbookConnection();

    void open();
    void reconnect();
    void close();

    bool isConnected() const;
    bool sendCommand(std::vector<unsigned char> serialized);

signals:
    void connected();
    void disconnected();
    void connectionError(QAbstractSocket::SocketError error);
    void payloadReceived(QByteArray serialized);

public slots:

private slots:
    void onTryAutoConnect();
    void onConnected();
    void onDisconnected();
    void onConnectionError(QAbstractSocket::SocketError error);
    void onMessageReceived(QByteArray message);

private:
    void init();
    void maybeStartReconnecting();

private:
    QPointer<QTimer> _autoConnectTimer;
    QPointer<QWebSocket> _socket;
    const QNetworkRequest _request;
    bool _closed{ false };
};
}

#endif // ORDERBOOKCONNECTION_HPP
