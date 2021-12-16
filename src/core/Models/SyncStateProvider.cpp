#include "SyncStateProvider.hpp"

//==============================================================================

SyncStateProvider::SyncStateProvider(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

SyncStateProvider::~SyncStateProvider() {}

//==============================================================================

bool SyncStateProvider::syncing() const
{
    return _syncing;
}

//==============================================================================

void SyncStateProvider::setSyncing(const bool sync)
{
    if (syncing() != sync) {
        _syncing = sync;
        emit syncingChanged();
    }
}

//==============================================================================

bool SyncStateProvider::scanning() const
{
    return _scanning;
}

//==============================================================================

void SyncStateProvider::setScanning(const bool scan)
{
    if (_scanning != scan) {
        _scanning = scan;
        emit scanningChanged();
    }
}

//==============================================================================

unsigned SyncStateProvider::bestBlockHeight() const
{
    return _bestBlockHeight;
}

//==============================================================================

void SyncStateProvider::setBestBlockHeight(unsigned bestBlockHeight)
{
    if (_bestBlockHeight != bestBlockHeight) {
        _bestBlockHeight = bestBlockHeight;
        bestBlockHeightChanged();
    }
}

//==============================================================================

unsigned SyncStateProvider::rescanProgress() const
{
    return _blocks;
}

//==============================================================================

void SyncStateProvider::setRescanProgress(unsigned blocks)
{
    if (_blocks != blocks) {
        _blocks = blocks;
        rescanProgressChanged();
    }
}

//==============================================================================
