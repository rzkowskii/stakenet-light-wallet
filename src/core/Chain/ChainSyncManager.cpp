#include "ChainSyncManager.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/BlockFilterMatcher.hpp>
#include <Chain/Chain.hpp>
#include <Chain/ChainSyncHelper.hpp>
#include <Models/WalletDataSource.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Utils/Logging.hpp>
#include <Utils/Utils.hpp>

//==============================================================================

static bool IsReorg(std::vector<NetworkUtils::ApiErrorException::ApiError> errors)
{
    for (auto&& error : errors) {
        if (error.message.contains("Block not found")) {
            return true;
        }
    }

    return false;
}

//==============================================================================

ChainSyncManager::ChainSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
    WalletDataSource& walletDataSource, const BlockFilterMatchable& blockFilterMatcher,
    CoinAsset asset, AbstractChainDataSource& chainDataSource, QObject* parent)
    : AbstractChainSyncManager(chain, transactionsCache, asset, parent)
    , _filterMatcher(blockFilterMatcher)
    , _walletDataSource(walletDataSource)
    , _chainDataSource(chainDataSource)

{
}

//==============================================================================

ChainSyncManager::~ChainSyncManager() {}

//==============================================================================

void ChainSyncManager::trySync()
{
    QMetaObject::invokeMethod(this, [this] {
        auto bbHash = chain().bestBlockHash();
        if (bbHash.isEmpty()) {
            syncBestBlockHash();
        } else {
            scheduleLastHeaders(bbHash);
        }
    });
}

//==============================================================================

void ChainSyncManager::interruptAsync()
{
    Utils::ScheduleJob(
        this, std::bind(&ChainSyncManager::setIsSyncing, std::placeholders::_1, false));
    LogCDebug(Sync) << "ChainSyncManager task interrupted";
}

//==============================================================================

void ChainSyncManager::interrupt()
{
    interruptAsync();
}

//==============================================================================

void ChainSyncManager::onHeadersSynced(std::vector<Wire::VerboseBlockHeader> headers)
{
    // discard result if we are not syncing anymore
    if (!isSyncing()) {
        return;
    }

    if (!headers.empty()) {
        for (auto&& header : headers) {
            _headersProcessingQueue.push(header);
        }

        syncProgress(static_cast<unsigned>(headers.back().height));
        processHeaders();
    } else {
        LogCDebug(Sync) << "Got sequence of headers with unknown prevBlockHash";
        Q_ASSERT(_headersProcessingQueue
                     .empty()); // we should get here only if we don't have any work left
        setIsSyncing(false);
    }
}

//==============================================================================

void ChainSyncManager::onAPISyncError(QString error)
{
    LogCCritical(Sync) << "API Sync error" << error;
    while (!_headersProcessingQueue.empty()) {
        _headersProcessingQueue.pop();
    }
    _pendingBlocks.clear();
    setIsSyncing(false);
    syncError(error);
}

//==============================================================================

void ChainSyncManager::onStrippedBlockSynced(Wire::StrippedBlock block)
{
    //    LogCDebug(Sync) << "onStrippedBlockSynced" << block.hash;

    auto it = _pendingBlocks.find(block.hash);
    if (it != std::end(_pendingBlocks)) {
        it->second = block;
        processHeaders();
    }
}

//==============================================================================

void ChainSyncManager::syncBestBlockHash()
{
    setIsSyncing(true);
    auto assetID = chain().assetID();
    _chainDataSource.getBestBlockHash(assetID)
        .then([this, assetID](std::tuple<BlockHash, BlockHeight> chainInfo) {
            auto bestBlockHeight = std::get<1>(chainInfo);
            auto reorgDepth = std::max<long int>(0, bestBlockHeight - GetMaxReorgDepth(false));
            return _chainDataSource.getBlockHash(assetID, reorgDepth)
                .then([this](BlockHash blockHash) { scheduleLastHeaders(blockHash); });
        })
        .fail([this](
                  NetworkUtils::ApiErrorException& error) { onAPISyncError(error.errorResponse); });
}

//==============================================================================

void ChainSyncManager::scheduleLastHeaders(QString bestBlockHash)
{
    setIsSyncing(true);
    _chainDataSource.getBlockHeaders(chain().assetID(), bestBlockHash)
        .then([this](std::vector<Wire::VerboseBlockHeader> blockHeaders) {
            onHeadersSynced(blockHeaders);
        })
        .fail([this, bestBlockHash](NetworkUtils::ApiErrorException& error) {
            if (IsReorg(error.errors)) {
                reorg(bestBlockHash);
            } else {
                onAPISyncError(error.errorResponse);
            }
        });
}

//==============================================================================

const BlockFilterMatcher& ChainSyncManager::filterMatcher()
{
    if (!_cachedMatcher) {
        _cachedMatcher = _filterMatcher.createMatcher(chain().assetID());
    }

    return *_cachedMatcher;
}

//==============================================================================

void ChainSyncManager::setBestBlockHeight(size_t bestBlockHeight)
{
    if (_bestBlockHeight != bestBlockHeight) {
        _bestBlockHeight = bestBlockHeight;
        bestBlockHeightChanged(static_cast<unsigned>(_bestBlockHeight));
    }
}

//==============================================================================

size_t ChainSyncManager::bestBlockHeight() const
{
    return _bestBlockHeight;
}

//==============================================================================

void ChainSyncManager::processHeaders()
{
    boost::optional<Wire::VerboseBlockHeader> lastAddedHeader;
    // TODO: Check this code when reorg happens.
    while (!_headersProcessingQueue.empty()) {
        lastAddedHeader = _headersProcessingQueue.front();
        QString blockHash = QString::fromStdString(lastAddedHeader->hash);
        // means we are waiting for stripped block
        auto index = BlockIndex(lastAddedHeader.get());
        if (_pendingBlocks.count(blockHash) > 0) {
            if (auto strippedBlock = _pendingBlocks.at(blockHash)) {
                std::vector<Transaction> appliedTxns;
                auto lookupTx = [&appliedTxns](QString txId) {
                    auto it = std::find_if(
                        std::begin(appliedTxns), std::end(appliedTxns), [txId](const auto& it) {
                            return boost::variant2::get<OnChainTxRef>(it)->txId() == txId;
                        });

                    return it != std::end(appliedTxns) ? boost::variant2::get<OnChainTxRef>(*it)
                                                       : OnChainTxRef{};
                };

                for (auto&& ref : strippedBlock->transactions) {
                    if (auto appliedTransaction
                        = _walletDataSource.applyTransaction(ref, lookupTx)) {
                        if (!appliedTransaction->outputs().empty()) {
                            _cachedMatcher.reset();
                        }

                        appliedTxns.emplace_back(appliedTransaction);
                    }
                }
                txCache().addTransactionsSync(appliedTxns);
            } else {
                // means it wasn't delivered, since we do everything in order. We need to come here
                // later.
                return;
            }
        } else if (filterMatcher().match(index)) {
            // if we found a matched block, we wait for processing and return here later
            scheduleStrippedBlock(index.hash(), index.height());
            return;
        }

        _headersProcessingQueue.pop();
        auto lastHeadersHeight = lastAddedHeader->height;
        if (chain().getHeight() < lastHeadersHeight) {
            chain().connectTip(*lastAddedHeader);

            if (bestBlockHeight() < lastHeadersHeight) {
                setBestBlockHeight(lastHeadersHeight);
            }
        }
    }

    if (_headersProcessingQueue.empty() && lastAddedHeader) {
        scheduleLastHeaders(QString::fromStdString(lastAddedHeader->hash));
    }
}

//==============================================================================

void ChainSyncManager::reorg(BlockHash reorgHash)
{
    LogCCInfo(Sync) << "Reorg detected at block:" << reorgHash << chain().getHeight();
    auto orphanHeight = [this, reorgHash]() -> size_t {
        while (auto header = chain().headerAt(chain().getHeight())) {
            if (header->hash == reorgHash.toStdString()) {
                return header->height;
            }
        }

        return 0;
    }();

    if (orphanHeight == 0) {
        LogCCritical(Sync) << "Can't reorg, reached end of local cache with hash" << reorgHash;
        syncError(QString("Can't reorg, reached end of local cache"));
        return;
    }

    const auto maxReorgDepth
        = std::max<long int>(1, chain().getHeight() - GetMaxReorgDepth(false) + 1);
    if (auto header = chain().headerAt(maxReorgDepth)) {
        findForkBlock(QString::fromStdString(header->hash), orphanHeight, {})
            .then([this](ForkBlock forkBlock) {
                BlockHash forkBlockHash;
                size_t forkHeight;
                std::vector<Wire::VerboseBlockHeader> alreadyFetchedHeaders;
                std::tie(forkBlockHash, forkHeight, alreadyFetchedHeaders) = forkBlock;

                LogCDebug(Sync) << "Found fork at block" << forkBlockHash << forkHeight;
                while (chain().bestBlockHash() != forkBlockHash) {
                    auto disconnectedBlockHash = chain().bestBlockHash();
                    chain().disconnectTip();
                    for (auto&& txid : txCache().transactionsInBlockSync(disconnectedBlockHash)) {
                        auto tx = txCache().transactionByIdSync(txid);
                        _walletDataSource.undoTransaction(tx);
                        tx->invalidateBlockHeight();
                        txCache().addTransactionsSync({ tx }); // we will update height here
                    }
                }

                auto newBestBlockHash = chain().bestBlockHash().toStdString();

                auto it = std::find_if(std::begin(alreadyFetchedHeaders),
                    std::end(alreadyFetchedHeaders), [newBestBlockHash](const auto& header) {
                        return header.hash == newBestBlockHash;
                    });

                if (it != std::end(alreadyFetchedHeaders)) {
                    alreadyFetchedHeaders.erase(std::begin(alreadyFetchedHeaders), it);
                    onHeadersSynced(alreadyFetchedHeaders);
                } else {
                    scheduleLastHeaders(chain().bestBlockHash());
                }
            })
            .reject(
                [this](NetworkUtils::ApiErrorException ex) { onAPISyncError(ex.errorResponse); })
            .reject(
                [this] { onAPISyncError(QString("Unknown error when trying to findForkBlock")); });
    } else {
        onAPISyncError(
            QString("Could not find header in local db with maxReorgDepth %1").arg(maxReorgDepth));
    }
}

//==============================================================================

auto ChainSyncManager::findForkBlock(BlockHash startSearchHash, size_t reorgHeight,
    std::vector<Wire::VerboseBlockHeader> cachedHeaders) const -> Promise<ForkBlock>
{
    return Promise<ForkBlock>([=](const auto& resolve, const auto& reject) {
        _chainDataSource.getBlockHeaders(this->chain().assetID(), startSearchHash)
            .then([this, reorgHeight, resolve, reject, cachedHeaders{ std::move(cachedHeaders) }](
                      std::vector<Wire::VerboseBlockHeader> headers) mutable {
                std::copy(
                    std::begin(headers), std::end(headers), std::back_inserter(cachedHeaders));
                auto it = std::find_if(std::rbegin(cachedHeaders), std::rend(cachedHeaders),
                    [reorgHeight](const auto& header) { return reorgHeight == header.height; });

                if (it != std::rend(cachedHeaders)) {
                    while (auto header = this->chain().headerAt(it->height)) {
                        if (header->hash == it->hash) {
                            resolve(ForkBlock{
                                QString::fromStdString(it->hash), it->height, cachedHeaders });
                            return;
                        }

                        ++it;
                    }

                    reject();
                } else {
                    // continue searching in case we didn't find block with needed height
                    this->findForkBlock(QString::fromStdString(headers.back().hash), reorgHeight,
                            std::move(cachedHeaders))
                        .then([resolve](ForkBlock block) { resolve(std::move(block)); })
                        .fail([reject](NetworkUtils::ApiErrorException& error) { reject(error); });
                }
            })
            .fail([reject](NetworkUtils::ApiErrorException& error) { reject(error); });
    });
}

//==============================================================================

void ChainSyncManager::scheduleStrippedBlock(BlockHash blockHash, int64_t blockHeight)
{
    _pendingBlocks.emplace(blockHash, boost::none);
    _chainDataSource.getLightWalletBlock(coinAsset().coinID(), blockHash, blockHeight)
        .then([=](Wire::StrippedBlock block) { onStrippedBlockSynced(block); })
        .fail([this](
                  NetworkUtils::ApiErrorException& error) { onAPISyncError(error.errorResponse); });
}

//==============================================================================

RescanSyncManager::RescanSyncManager(Chain& chain, AssetsTransactionsCache& transactionsCache,
    WalletDataSource& walletDataSource, const BlockFilterMatchable& blockFilterMatcher,
    CoinAsset asset, AbstractChainDataSource& chainDataSource, QObject* parent)
    : ChainSyncManager(chain, transactionsCache, walletDataSource, blockFilterMatcher, asset,
          chainDataSource, parent)
{
}

//==============================================================================

void RescanSyncManager::trySync()
{
    QMetaObject::invokeMethod(this, [this] {
        _chainDataSource.getBestBlockHash(coinAsset().coinID())
            .then([this](std::tuple<BlockHash, BlockHeight> chainInfo) {
                setBestBlockHeight(std::get<1>(chainInfo));
                scheduleLastHeaders(coinAsset().misc().rescanStartHash);
            })
            .fail([this](const NetworkUtils::ApiErrorException& error) {
                syncError(error.errorResponse);
            });
    });
}

//==============================================================================

void RescanSyncManager::interruptAsync()
{
    setIsSyncing(false);
    LogCDebug(Sync) << "RescanSyncManager task interrupted";
}

//==============================================================================
