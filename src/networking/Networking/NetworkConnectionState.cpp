#include "NetworkConnectionState.hpp"

#include <QNetworkAccessManager>
#include <QThread>

static QPointer<NetworkConnectionState> instance{ nullptr };

//==============================================================================

void NetworkConnectionState::Initialize()
{
    Q_ASSERT_X(
        !instance, "NetworkConnectionState::Initialize()", "Singletone is already initialized");
    instance = new NetworkConnectionState;
}

//==============================================================================

void NetworkConnectionState::Shutdown()
{
    Q_ASSERT_X(instance, "NetworkConnectionState::Shutdown()", "Singletone is not yet initialized");
    instance->deleteLater();
}

//==============================================================================

NetworkConnectionState* NetworkConnectionState::Singleton()
{
    Q_ASSERT_X(instance, "NetworkConnectionState::Singleton()", "Singletone not ready");
    return instance;
}

//==============================================================================

NetworkConnectionState::State NetworkConnectionState::state() const
{
    return _state;
}

//==============================================================================

void NetworkConnectionState::gossipConnectionState(NetworkConnectionState::State newState)
{
    // TODO(yuraolex): in future needs to be smarter, actually check if we have new state
    QMetaObject::invokeMethod(this, [=] { setState(newState); });
}

//==============================================================================

NetworkConnectionState::NetworkConnectionState(QObject* parent)
    : QObject(parent)
    , _networkAccessManagaer(new QNetworkAccessManager(this))
{
    qRegisterMetaType<NetworkConnectionState::State>("State");
}

//==============================================================================

void NetworkConnectionState::setState(NetworkConnectionState::State newState)
{
    if (_state != newState) {
        _state = newState;
        stateChanged(newState);
    }
}

//==============================================================================
