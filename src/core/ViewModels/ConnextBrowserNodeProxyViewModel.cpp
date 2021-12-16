#include "ConnextBrowserNodeProxyViewModel.hpp"
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

ConnextBrowserNodeProxyViewModel::ConnextBrowserNodeProxyViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

ConnextBrowserNodeApiTransport* ConnextBrowserNodeProxyViewModel::transport() const
{
    return _transport;
}

//==============================================================================

ConnextProcessManager* ConnextBrowserNodeProxyViewModel::processManager() const
{
    return _processManager;
}

//==============================================================================

void ConnextBrowserNodeProxyViewModel::initialize(ApplicationViewModel* appViewModel)
{
    auto connextDaemonManager = appViewModel->paymentNodesManager()->connextDaemonManager();
    _transport = connextDaemonManager->browserNodeApi()->transport();
    _processManager = connextDaemonManager->processManager();

    emit updated();
}

//==============================================================================
