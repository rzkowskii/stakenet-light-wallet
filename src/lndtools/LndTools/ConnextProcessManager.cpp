#include "ConnextProcessManager.hpp"
#include <Networking/DockerApiClient.hpp>
#include <Utils/Logging.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <optional>

//==============================================================================

ConnextProcessManager::ConnextProcessManager(ConnextDaemonConfig& daemonCfg, QObject* parent)
    : AbstractPaymentNodeProcessManager(parent)
    , _daemonCfg(daemonCfg)
{
}

//==============================================================================

void ConnextProcessManager::start()
{
    if (!running()) {
        emit requestStart();
    }
}

//==============================================================================

void ConnextProcessManager::stop()
{
    if (running()) {
        emit requestStop();
    }
}

//==============================================================================

QStringList ConnextProcessManager::getNodeConf() const
{
    return { _daemonCfg.hubHost };
}

//==============================================================================

void ConnextProcessManager::onBrowserNodeReady(bool ready)
{
    setRunning(ready);
}

//==============================================================================
