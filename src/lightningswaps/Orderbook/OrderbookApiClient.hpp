#ifndef ORDERBOOKAPICLIENT_HPP
#define ORDERBOOKAPICLIENT_HPP

#include <Networking/OrderbookConnection.hpp>
#include <Orderbook/Protos/stakenet/orderbook/api.pb.h>
#include <QAbstractSocket>
#include <QObject>
#include <Utils/Utils.hpp>
#include <unordered_map>

class QTimer;

namespace orderbook {

using io::stakenet::orderbook::protos::Command;
using io::stakenet::orderbook::protos::Event;
using OrderbookResponse = Event::CommandResponse;

class OrderbookApiClient : public QObject {
    Q_OBJECT
public:
    enum class State { Connected, Disconnected, Error, InMaintenance };
    explicit OrderbookApiClient(
        QUrl host, std::map<QString, QString> params, QObject* parent = nullptr);
    void open();
    void reconnect();
    void close();
    Promise<Event::CommandResponse> makeRequest(Command command);

signals:
    void notificationReceived(Event::ServerEvent event);
    void connected();
    void disconnected();
    void connectionError(QAbstractSocket::SocketError error);
    void stateChanged(State newState);

public slots:

private slots:
    void onPingTimeout();
    void onConnected();
    void onDisconnected();
    void onConnectionError(QAbstractSocket::SocketError error);
    void onDataReceived(QByteArray payload);
    void setState(State state);

private:
    void init(QUrl host, std::map<QString, QString> params);
    std::string generateId() const;

private:
    using RequestSender = std::tuple<QtPromise::QPromiseResolve<Event::CommandResponse>,
        QtPromise::QPromiseReject<Event::CommandResponse>>;

    std::unordered_map<std::string, RequestSender> _pendingCalls;
    OrderbookConnection* _orderbookConnection{ nullptr };
    QTimer* _pingTimer{ nullptr };
    int64_t _messageCounter{ 0 };
    QByteArray _lastMsgChecksum;
    State _state{ State::Disconnected };
};
}

#endif // ORDERBOOKAPICLIENT_HPP
