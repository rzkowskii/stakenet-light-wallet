#include "EmulatorChainSyncManager.hpp"
#include <Chain/Chain.hpp>
#include <Utils/Logging.hpp>
#include <Utils/Utils.hpp>
#include <ViewModels/EmulatorViewModel.hpp>

EmulatorChainSyncManager::EmulatorChainSyncManager(Chain& chain,
    AssetsTransactionsCache& transactionsCache, WalletDataSource& walletDataSource,
    const BlockFilterMatchable& blockFilterMatcher, CoinAsset asset,
    Utils::WorkerThread& workerThread,
    QPointer<AbstractBlockExplorerHttpClient> blockExplorerHttpClient,
    EmulatorViewModel& emulatorViewModel, QObject* parent)
    : AbstractChainSyncManager(chain, transactionsCache, asset, parent)
    , _filterMatcher(blockFilterMatcher)
    , _walletDataSource(walletDataSource)
    , _chainSyncHelper(nullptr /*new ChainSyncHelper(blockExplorerHttpClient, asset, this)*/)
    , _emulatorViewModel(emulatorViewModel)
{
    //    connect(syncHelper, &ChainSyncHelper::blockSynced, this,
    //    &EmulatedChainSyncManager::onStrippedBlockSynced);
}

//==============================================================================

void EmulatorChainSyncManager::trySync()
{
    QMetaObject::invokeMethod(this, [this] {
        auto bbHash = chain().bestBlockHash();
        scheduleLastHeaders(bbHash);
    });
}

//==============================================================================

void EmulatorChainSyncManager::interruptAsync()
{
    Utils::ScheduleJob(
        this, std::bind(&EmulatorChainSyncManager::setIsSyncing, std::placeholders::_1, false));
    LogCDebug(General) << "ChainSyncManager task interrupted";
}

//==============================================================================

void EmulatorChainSyncManager::interrupt()
{
    QMetaObject::invokeMethod(this, std::bind(&EmulatorChainSyncManager::setIsSyncing, this, false),
        Qt::BlockingQueuedConnection);
}

//==============================================================================

void EmulatorChainSyncManager::scheduleLastHeaders(QString bestBlockHash)
{
    setIsSyncing(true);
    setIsSyncing(false);
    //    Utils::ScheduleJob(_chainSyncHelper.get(),
    //                       std::bind(&ChainSyncHelper::getLastHeaders, std::placeholders::_1,
    //                       bestBlockHash));
}

//==============================================================================

void EmulatorChainSyncManager::onHeadersSynced(std::vector<Wire::VerboseBlockHeader> headers)
{
#if 0
    // discard result if we are not syncing anymore
    if(!isSyncing())
    {
        return;
    }

    if(!headers.empty())
    {
        boost::optional<BlockIndex> lastAddedIndex;
        // TODO: Check this code when reorg happens.
        for(auto &&header : headers)
        {
            auto index = BlockIndex(std::get<0>(header), std::get<1>(header));
            if(index.height() >= _emulatorViewModel.maxHeight())
            {
                break;
            }

            lastAddedIndex = index;
        }

        if(lastAddedIndex)
        {
            if(lastAddedIndex->height() >= chain().getHeight())
            {
                chain().setTip(lastAddedIndex->header().hash, lastAddedIndex->height());
            }

            if(lastAddedIndex->height() >= _emulatorViewModel.maxHeight())
            {
                setIsSyncing(false);
            }
            else
            {
                scheduleLastHeaders(lastAddedIndex->header().hash);
            }
        }
        else
        {
            setIsSyncing(false);
        }
    }
    else
    {
        LogCDebug(General) << "Got sequence of headers with unknown prevBlockHash";
        if(_pendingBlocks.empty())
        {
            setIsSyncing(false);
        }
    }
#endif
}

//==============================================================================

void EmulatorChainSyncManager::onAPISyncError(QString error)
{
    LogCDebug(General) << "API Sync error" << error;
    setIsSyncing(false);
    syncError(error);
}

//==============================================================================
