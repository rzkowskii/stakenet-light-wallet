#include "ChainManager.hpp"
#include <Chain/Chain.hpp>
#include <Tools/Bootstrap.hpp>
#include <Tools/Common.hpp>
#include <Tools/DBUtils.hpp>
#include <Utils/Logging.hpp>
#include <txdb.h>

#include <QTimer>

//==============================================================================

static constexpr size_t FLUSH_DISK_TIMER_INTERVAL = 100;
static constexpr size_t BLOCKS_STORED_LOCALLY = 10000;

//==============================================================================

ChainManager::ChainManager(const WalletAssetsModel& assetsModel, QString dataDir, QObject* parent)
    : AbstractMutableChainManager(assetsModel, parent)
    , _dataDir(dataDir.toStdString())
{
    _diskFlushTimer = new QTimer(this);
    _diskFlushTimer->setSingleShot(true);
    connect(_diskFlushTimer, &QTimer::timeout, this, &ChainManager::flushStateToDisk);
}

//==============================================================================

Promise<void> ChainManager::loadChains(std::vector<AssetID> chains)
{
    return Promise<void>([=](const auto& resolver, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            try {
                _chainDB.reset(new bitcoin::CBlockTreeDB(_dataDir, 1000, false, false));
                this->executeLoadChains(chains);
                this->chainsLoaded(chains);
                resolver();
            } catch (std::exception& ex) {
                LogCCritical(Chains) << "Failed to load chains cache:" << ex.what();
                reject(ex);
            }
        });
    });
}

//==============================================================================

Promise<void> ChainManager::loadFromBootstrap(QString bootstrapFile)
{
    return Promise<void>([=](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(this, [=] {
            this->executeLoadBootstrap(bootstrapFile);
            resolver();
        });
    });
}

//==============================================================================

void ChainManager::executeLoadChains(std::vector<AssetID> chains)
{
    std::vector<AssetID> chainsToLoad;

    std::copy_if(std::begin(chains), std::end(chains), std::back_inserter(chainsToLoad),
        [this](const auto& id) { return _chains.count(id) == 0; });

    if (chainsToLoad.empty()) {
        LogCDebug(Chains) << "chainsToLoad is empty";
        return;
    }

    auto setNewTip = [this](AssetID assetID, Wire::VerboseBlockHeader newTip) {
        _dirtyIndex[assetID].emplace_back(newTip);
        startFlushTimer();
    };

    auto getHeader
        = [this](AssetID assetID, size_t height) -> boost::optional<Wire::VerboseBlockHeader> {
        if (height == 0) // won't handle genesis blocks
        {
            return boost::none;
        }

        bitcoin::CDiskBlockIndex index;

        auto it = _dirtyIndex.find(assetID);
        if (it != std::end(_dirtyIndex)) {
            const auto& list = it->second;
            auto lIt = std::find_if(std::begin(list), std::end(list),
                [height](const auto& item) { return item.height == height; });

            if (lIt != std::end(list)) {
                return *lIt;
            }
        }

        if (_chainDB->ReadBlockIndex(assetID, height, index)) {
            return { DBUtils::DiskIndexToBlockIndex(index) };
        }

        return boost::none;
    };

    LogCDebug(Chains) << "chainsToLoad " << chainsToLoad.size();
    for (auto&& id : chainsToLoad) {
        auto set = std::bind(setNewTip, id, std::placeholders::_1);
        auto get = std::bind(getHeader, id, std::placeholders::_1);
        _chains.emplace(id, std::unique_ptr<Chain>(new Chain(id, set, get)));
    }

    _chainDB->LoadBestBlockGuts(
        [this, &chainsToLoad](AssetID assetID, uint32_t bestHeight) {
            if (_chains.count(assetID) == 0) {
                return;
            }

            if (std::find(std::begin(chainsToLoad), std::end(chainsToLoad), assetID)
                == std::end(chainsToLoad)) {
                return;
            }

            if (bestHeight > 0) {
                bitcoin::CDiskBlockIndex diskIndex;
                if (_chainDB->ReadBlockIndex(assetID, bestHeight, diskIndex)) {
                    auto& chain = *_chains.at(assetID);
                    chain.connectTip(DBUtils::DiskIndexToBlockIndex(diskIndex));
                } else {
                    LogCCritical(Chains) << "No block index found for asset:" << assetID
                                         << "best height:" << bestHeight;
                }
            }
        },
        chainsToLoad);

    LogCDebug(Chains) << "Chain indexes loaded";
}

//==============================================================================

void ChainManager::onFlushError(QString error)
{
    LogCWarning(Chains) << "Chain flusshing error" << error;
}

//==============================================================================

void ChainManager::flushStateToDisk()
{
    for (auto&& it : _dirtyIndex) {
        auto& items = it.second;
        bitcoin::CDBBatch batch(*_chainDB);
        uint32_t bestBlockHeight = 0;
        for (auto&& item : items) {
            _chainDB->WriteBlockIndex(batch, it.first, DBUtils::BlockIndexToDisk(item));
            bestBlockHeight = std::max(item.height, bestBlockHeight);
        }

        uint32_t storedBBHeight = 0;
        if (_chainDB->ReadBestBlock(it.first, storedBBHeight)) {
            bestBlockHeight = std::max(storedBBHeight, bestBlockHeight);
        }

        _chainDB->WriteBestBlock(batch, it.first, bestBlockHeight);
        _chainDB->WriteBatch(batch, true);

        const auto staleHeight = bestBlockHeight - BLOCKS_STORED_LOCALLY - items.size();
        for (size_t i = 0; i < items.size(); ++i) {
            _chainDB->EraseBlockIndex(batch, it.first, staleHeight + i);
        }

        _chainDB->WriteBatch(batch, true);
        items.clear();
    }

    _dirtyIndex.clear();
}

//==============================================================================

void ChainManager::startFlushTimer()
{
    if (!_diskFlushTimer->isActive()) {
        _diskFlushTimer->start(FLUSH_DISK_TIMER_INTERVAL);
    }
}

//==============================================================================

void ChainManager::executeLoadBootstrap(QString path)
{
#if 0
    BootstrapReader bootstrapReader(path);
    std::vector<std::pair<AssetID, bitcoin::CDiskBlockIndex>> diskIndexes;
    diskIndexes.reserve(10000);

    while(bootstrapReader.canReadMore())
    {
        auto assetReader = bootstrapReader.readNextAsset();
        bitcoin::uint256 bestBlockHash;

        if(_chainDB->Exists(assetReader->assetID))
        {
            _chainDB->EraseByAsset(assetReader->assetID);
        }

        while(assetReader->canReadMore())
        {
            diskIndexes.push_back({assetReader->assetID, assetReader->readNext()});

            if(assetReader->remainingBlocks() == 0)
            {
                bestBlockHash = diskIndexes.back().second.hashBlock;
            }

            if(diskIndexes.size() >= 10000)
            {
                _chainDB->WriteBatchSync(diskIndexes);
                diskIndexes.clear();
            }
        }

        if(!diskIndexes.empty())
        {
            _chainDB->WriteBatchSync(diskIndexes);
            diskIndexes.clear();
        }

        bitcoin::CDBBatch batch(*_chainDB);
        _chainDB->WriteBestBlockBatch(batch, assetReader->assetID, bestBlockHash);
        _chainDB->WriteBatch(batch, true);
    }

    bootstrapReader.finalize();

    LogCDebug(Chains) << "Bootstrap loaded";
#endif
}

//==============================================================================
