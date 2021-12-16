#include "DaemonMonitor.hpp"
#include <LndTools/AbstractPaymentNodeProcessManager.hpp>

//==============================================================================

DaemonMonitor::DaemonMonitor(AbstractPaymentNodeProcessManager* processManager,
    std::function<bool()> canStart, QObject* parent)
    : QObject(parent)
    , _processManager(processManager)
    , _restartTimer(new QTimer(this))
    , _canStart(canStart)
{
    _restartTimer->setInterval(5000);
    _restartTimer->setSingleShot(false);
    connect(_restartTimer, &QTimer::timeout, this, &DaemonMonitor::onCheckState);
}

//==============================================================================

DaemonMonitor::~DaemonMonitor() {}

//==============================================================================

Promise<void> DaemonMonitor::start()
{
    _restartTimer->start();
    return QtPromise::resolve();
}

//==============================================================================

Promise<void> DaemonMonitor::stop()
{
    _restartTimer->stop();
    if (_stopTimer) {
        return Promise<void>::reject(std::runtime_error("Already stopping"));
    }

    if (_processManager && _processManager->running()) {
        _stopTimer = new QTimer(this);
        _stopTimer->setInterval(500);
        _stopTimer->setSingleShot(false);
        connect(_stopTimer, &QTimer::timeout, _processManager.data(),
            &AbstractPaymentNodeProcessManager::stop);
        _stopTimer->start(_stopTimer->interval());
        return QtPromise::connect(
            _processManager.data(), &AbstractPaymentNodeProcessManager::runningChanged)
            .finally([timer = _stopTimer] { timer->deleteLater(); });
    } else {
        return Promise<void>::resolve();
    }
}

//==============================================================================

void DaemonMonitor::onCheckState()
{
    if (_processManager) {
        if (!_processManager->running()) {
            if (_canStart && !_canStart()) {
                return;
            }

            _processManager->start();
        }
    }
}

//==============================================================================
