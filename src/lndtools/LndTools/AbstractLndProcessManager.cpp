#include "AbstractLndProcessManager.hpp"

//==============================================================================

AbstractLndProcessManager::AbstractLndProcessManager(QObject* parent)
    : AbstractPaymentNodeProcessManager(parent)
{
}

//==============================================================================

AbstractLndProcessManager::~AbstractLndProcessManager()
{
    if (!_lndProcess.waitForFinished(5000)) {
        _lndProcess.terminate();
    }
}

//==============================================================================
