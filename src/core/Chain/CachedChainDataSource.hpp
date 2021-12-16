#ifndef CACHEDCHAINDATASOURCE_HPP
#define CACHEDCHAINDATASOURCE_HPP

#include <Chain/AbstractChainDataSource.hpp>
#include <Chain/ChainSyncHelper.hpp>
#include <QObject>
#include <Utils/Utils.hpp>
#include <boost/compute/detail/lru_cache.hpp>
#include <unordered_map>

class ChainSyncHelper;
class WalletAssetsModel;
class AbstractNetworkingFactory;
class AssetsTransactionsCache;
class AbstractMutableChainManager;

//==============================================================================

class CachedChainDataSource : public AbstractChainDataSource {
    Q_OBJECT
public:
    explicit CachedChainDataSource(AbstractNetworkingFactory& networkingFactory,
        const WalletAssetsModel& assetsModel, AssetsTransactionsCache& transactionsCache,
        const AbstractMutableChainManager& chainManager, QObject* parent = nullptr);

    Promise<std::vector<Wire::VerboseBlockHeader>> getBlockHeaders(
        AssetID assetID, BlockHash startingBlockHash) const override;
    Promise<Wire::VerboseBlockHeader> getBlockHeader(
        AssetID assetID, BlockHash hash) const override;
    Promise<BlockHash> getBlockHash(AssetID assetID, size_t blockHeight) const override;
    Promise<std::tuple<BlockHash, BlockHeight>> getBestBlockHash(AssetID assetID) const override;
    Promise<std::vector<unsigned char>> getBlock(AssetID assetID, BlockHash hash) const override;
    Promise<Wire::StrippedBlock> getLightWalletBlock(
        AssetID assetID, BlockHash hash, int64_t blockHeight) const override;
    Promise<std::vector<std::string>> getFilteredBlock(
        AssetID assetID, BlockHash hash) const override;
    Promise<Wire::EncodedBlockFilter> getBlockFilter(
        AssetID assetID, BlockHash hash) const override;
    Promise<Wire::TxConfrimation> getRawTransaction(AssetID assetID, QString txid) const override;
    Promise<std::string> getRawTxByIndex(
        AssetID assetID, int64_t blockNum, uint32_t txIndex) const override;
    Promise<boost::optional<Wire::TxOut>> getTxOut(
        AssetID assetID, Wire::OutPoint outpoint) const override;
    Promise<QString> sendRawTransaction(AssetID assetID, QString serializedTx) const override;
    Promise<void> loadSecondLayerCache(
        AssetID assetID, uint32_t startBlock, Interrupt* onInterrupt) const override;
    Promise<void> freeSecondLayerCache(AssetID assetID) const override;
    Promise<int64_t> estimateNetworkFee(AssetID assetID, uint64_t blocks) const override;
    Promise<Wire::TxConfrimation> getSpendingDetails(
        AssetID assetID, Wire::OutPoint outpoint) const override;

private:
    ChainSyncHelper& chainSyncHelper(AssetID assetID) const;
    AbstractBlockExplorerHttpClient* apiClient(AssetID assetID) const;
    boost::optional<Wire::VerboseBlockHeader> fetchFromSecondLayerCache(
        AssetID assetID, BlockHash hash) const;
    boost::optional<Wire::VerboseBlockHeader> lookupHeaderFromCache(
        AssetID assetID, BlockHash hash) const;

private:
    QObject* _executionContext{ nullptr };
    AbstractNetworkingFactory& _networkingFactory;
    const WalletAssetsModel& _assetsModel;
    AssetsTransactionsCache& _transactionsCache;
    const AbstractMutableChainManager& _chainManager;

    using BlockHeadersCache = boost::compute::detail::lru_cache<std::pair<AssetID, BlockHash>,
        Wire::VerboseBlockHeader>;
    using SecondLayerBlockCache = std::unordered_map<BlockHash, Wire::VerboseBlockHeader>;
    using SecondLayerCache = std::map<AssetID, SecondLayerBlockCache>;
    using RawTransactionsCache
        = boost::compute::detail::lru_cache<std::pair<AssetID, TxID>, Wire::TxConfrimation>;
    using ChainSyncHelperPtr = qobject_delete_later_unique_ptr<ChainSyncHelper>;
    using BlockExplorerClientPtr = qobject_delete_later_unique_ptr<AbstractBlockExplorerHttpClient>;
    mutable std::map<AssetID, ChainSyncHelperPtr> _chainSyncHelpers;
    mutable std::map<AssetID, BlockExplorerClientPtr> _clients;
    mutable BlockHeadersCache _blockHeadersCache;
    mutable RawTransactionsCache _rawTranscationsCache;
    mutable SecondLayerCache
        _secondLayerBlockHeadersCache; // temporary loaded cache to sync lnd quickly
    mutable std::map<AssetID, std::unordered_map<size_t, BlockHash>> _secondLayerBlockHashIndex;
};

#endif // CACHEDCHAINDATASOURCE_HPP
