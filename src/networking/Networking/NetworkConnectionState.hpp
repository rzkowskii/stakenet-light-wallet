#ifndef NETWORKCONNECTIONSTATE_HPP
#define NETWORKCONNECTIONSTATE_HPP

#include <QObject>
#include <QPointer>

class QNetworkAccessManager;

/*!
 * \brief The NetworkConnectionState class tracks network connection state and notifies
 * subscribers about changes
 */
class NetworkConnectionState : public QObject {
    Q_OBJECT
public:
    enum class State { Connected, Disconnected };

    static void Initialize();
    static void Shutdown();
    static NetworkConnectionState* Singleton();

    State state() const;

    void gossipConnectionState(State newState);

signals:
    void stateChanged(State newState);

private:
    explicit NetworkConnectionState(QObject* parent = nullptr);

    void setState(State newState);

private:
    State _state{ State::Connected };
    QPointer<QNetworkAccessManager> _networkAccessManagaer;
};

#endif // NETWORKCONNECTIONSTATE_HPP
