#include "TorManager.hpp"
#include <ViewModels/ApplicationViewModel.hpp>
#define Q_OS_ANDROID // TODO(yuraolex): remove this!!!!!
#ifndef Q_OS_ANDROID
#include <TorController.hpp>
#endif
#include <QDir>
#include <QNetworkProxy>
#include <QSettings>
#include <Utils/Logging.hpp>

static const QString SETTINGS_TOR_ACTIVE_STATE("torActiveState");
static bool isInitialized = false;

//==============================================================================

TorManager::TorManager(QObject* parent)
    : QObject(parent)
{
    readSettings();
}

//==============================================================================

TorManager::~TorManager()
{
    stopTor();
}

//==============================================================================

TorManager* TorManager::Instance()
{
    static TorManager instance;
    return &instance;
}

//==============================================================================

bool TorManager::torActiveState() const
{
    return _torActiveState;
}

//==============================================================================

void TorManager::changeTorState()
{
    if (_torActiveState) {
        stopTor();
        LogCDebug(General) << "Tor is nonactive!";
    } else {
        startTor();
        LogCDebug(General) << "Tor is active!";
    }
    _torActiveState = !_torActiveState;
    writeSettings();
    torActiveStateChanged();
}

//==============================================================================

void TorManager::writeSettings() const
{
    QSettings settings;
    settings.setValue(SETTINGS_TOR_ACTIVE_STATE, QVariant::fromValue(_torActiveState));
    settings.sync();
}

//==============================================================================

void TorManager::readSettings()
{
    QSettings settings;
    _torActiveState = settings.value(SETTINGS_TOR_ACTIVE_STATE).toBool();
    if (_torActiveState) {
        startTor();
    } else {
        QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
    }
    torActiveStateChanged();
}

//==============================================================================

void TorManager::startTor()
{
#ifndef Q_OS_ANDROID
    QNetworkProxy proxy;
    proxy.setType(QNetworkProxy::Socks5Proxy);
    proxy.setHostName("127.0.0.1");
    proxy.setPort(9090);
    proxy.setCapabilities(QNetworkProxy::HostNameLookupCapability | proxy.capabilities());
    QNetworkProxy::setApplicationProxy(proxy);
    if (!isInitialized) {
        InitalizeTorThread(GetDataDir(ApplicationViewModel::Instance()->isEmulated()).path());
        isInitialized = true;
        LogCDebug(General) << "Tor initialized!";
    }
#endif
}

//==============================================================================

void TorManager::stopTor()
{
#ifndef Q_OS_ANDROID
    StopTorController();
    QNetworkProxy::setApplicationProxy(QNetworkProxy::NoProxy);
#endif
}

//==============================================================================
