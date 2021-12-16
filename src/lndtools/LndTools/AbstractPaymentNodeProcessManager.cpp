#include "AbstractPaymentNodeProcessManager.hpp"

//==============================================================================

AbstractPaymentNodeProcessManager::AbstractPaymentNodeProcessManager(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

bool AbstractPaymentNodeProcessManager::running() const
{
    return _running;
}

//==============================================================================

void AbstractPaymentNodeProcessManager::setRunning(bool running)
{
    if (_running != running) {
        _running = running;
        emit runningChanged();
    }
}

//==============================================================================
