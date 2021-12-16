#include "OrderbookApiClient.hpp"
#include <Networking/NetworkConnectionState.hpp>
#include <Orderbook/Types.hpp>
#include <Utils/Logging.hpp>

#include <QTimer>
#include <QUuid>

namespace orderbook {

//==============================================================================

static bool IsDuplicateMsg(const QByteArray& payload, const QByteArray& lastMsgChecksum)
{
    return QCryptographicHash::hash(payload, QCryptographicHash::Sha1) == lastMsgChecksum;
}

//==============================================================================

OrderbookApiClient::OrderbookApiClient(
    QUrl host, std::map<QString, QString> params, QObject* parent)
    : QObject(parent)
{
    init(host, params);
}

//==============================================================================

void OrderbookApiClient::open()
{
    _orderbookConnection->open();
}

//==============================================================================

void OrderbookApiClient::reconnect()
{
    _orderbookConnection->reconnect();
}

//==============================================================================

void OrderbookApiClient::close()
{
    _orderbookConnection->close();
}

//==============================================================================

Promise<Event::CommandResponse> OrderbookApiClient::makeRequest(Command command)
{
    return Promise<Event::CommandResponse>([=](const auto& resolve, const auto& reject) mutable {
        if (_orderbookConnection->isConnected()) {
            auto seq = this->generateId();
            command.set_clientmessageid(seq);
            LogCDebug(Orderbook) << "Sending orderbook request:" << command.DebugString().data()
                                 << seq.data();
            _pendingCalls.emplace(seq, RequestSender{ resolve, reject });
            _orderbookConnection->sendCommand(ProtobufSerializeToArray(command));
        } else {
            LogCDebug(Orderbook) << "Failing command, no connection:"
                                 << command.DebugString().data();
            reject(std::runtime_error("Api No Connection"));
        }
    });
}

//==============================================================================

void OrderbookApiClient::onPingTimeout()
{
    if (!_orderbookConnection->isConnected()) {
        return;
    }

    Command pingCmd;
    auto ping = new io::stakenet::orderbook::protos::PingCommand;
    pingCmd.set_allocated_ping(ping);
    _orderbookConnection->sendCommand(ProtobufSerializeToArray(pingCmd));
}

//==============================================================================

void OrderbookApiClient::onConnected()
{
    _pingTimer->start(_pingTimer->interval());
    _messageCounter = 0;
    _lastMsgChecksum.clear();
    connected();
}

//==============================================================================

void OrderbookApiClient::onDisconnected()
{
    _pingTimer->stop();

    for (auto&& call : _pendingCalls) {
        std::get<1>(call.second)(std::runtime_error("Orderbook disconnected"));
    }
    _pendingCalls.clear();

    if (_state != State::Error) {
        setState(State::Disconnected);
    }

    disconnected();
}

//==============================================================================

void OrderbookApiClient::onConnectionError(QAbstractSocket::SocketError error)
{
    _pingTimer->stop();

    for (auto&& call : _pendingCalls) {
        std::get<1>(call.second)(
            std::runtime_error("Orderbook connection error: " + std::to_string(error)));
    }
    _pendingCalls.clear();

    setState(State::Error);
    connectionError(error);
}

//==============================================================================

void OrderbookApiClient::onDataReceived(QByteArray payload)
{
    Event event;
    if (!event.ParseFromArray(payload.data(), payload.size())) {
        LogCDebug(Orderbook) << "Received unknown event";
        return;
    }

    LogCDebug(Orderbook) << "Received payload:" << event.DebugString().data();

    try {
        if ((_messageCounter + 1) != event.messagecounter()) {
            if (IsDuplicateMsg(payload, _lastMsgChecksum)) {
                return;
            } else {
                LogCCritical(Orderbook) << "Detected missed messages, state:" << _messageCounter
                                        << "vs" << event.messagecounter();
                _orderbookConnection->reconnect();
                return;
            }
        }

        ++_messageCounter;
        _lastMsgChecksum = QCryptographicHash::hash(payload, QCryptographicHash::Sha1);

        if (event.value_case() == Event::ValueCase::kResponse) {
            auto response = event.response();
            if (response.value_case() == Event::CommandResponse::ValueCase::kPingResponse) {
                return;
            }

            auto it = _pendingCalls.find(response.clientmessageid());
            if (it != std::end(_pendingCalls)) {
                auto sender = it->second;
                auto uid = it->first;
                _pendingCalls.erase(it);
                if (response.value_case() == Event::CommandResponse::ValueCase::kCommandFailed) {
                    std::string reason;
                    if (response.commandfailed().has_serverinmaintenance()) {
                        reason = "Server maintenance";
                        setState(State::InMaintenance);
                    } else {
                        reason = response.commandfailed().reason();
                    }
                    LogCCritical(Orderbook)
                        << "Command:" << uid.data() << "failed, reason:" << reason.c_str();
                    std::get<1>(sender)(reason);
                } else {
                    std::get<0>(sender)(response);
                }
            }
        } else if (event.value_case() == Event::ValueCase::kEvent) {
            if (event.event().has_maintenanceinprogress()) {
                setState(State::InMaintenance);
            } else if (event.event().has_maintenancecompleted()) {
                setState(State::Connected);
            }

            notificationReceived(event.event());
        } else {
            LogCDebug(Orderbook) << "Received unknown value case for event:" << event.value_case();
        }
    } catch (std::exception& ex) {
        LogCCritical(Orderbook) << "Exception while parsing onDataReceived:" << ex.what();
    }
}

//==============================================================================

void OrderbookApiClient::init(QUrl host, std::map<QString, QString> params)
{
    qRegisterMetaType<State>("State");
    _orderbookConnection
        = new OrderbookConnection(host, NetworkConnectionState::Singleton(), params, this);
    _pingTimer = new QTimer(this);
    _pingTimer->setInterval(30000);
    _pingTimer->setSingleShot(false);
    connect(_pingTimer, &QTimer::timeout, this, &OrderbookApiClient::onPingTimeout);
    connect(_orderbookConnection, &OrderbookConnection::connected, this,
        &OrderbookApiClient::onConnected);
    connect(_orderbookConnection, &OrderbookConnection::disconnected, this,
        &OrderbookApiClient::onDisconnected);
    connect(_orderbookConnection, &OrderbookConnection::connectionError, this,
        &OrderbookApiClient::onConnectionError);
    connect(_orderbookConnection, &OrderbookConnection::payloadReceived, this,
        &OrderbookApiClient::onDataReceived);
}

//==============================================================================

void OrderbookApiClient::setState(State state)
{
    if (_state != state) {
        _state = state;
        stateChanged(state);
    }
}

//==============================================================================

std::string OrderbookApiClient::generateId() const
{
    std::string id;
    do {
        id = QUuid::createUuid().toString().toStdString();
    } while (_pendingCalls.count(id) > 0);

    return id;
}

//==============================================================================
}
