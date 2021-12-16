#ifndef REGTESTCHAIN_HPP
#define REGTESTCHAIN_HPP

#include <Chain/AbstractChainDataSource.hpp>
#include <Chain/AbstractChainManager.hpp>
#include <Models/WalletDataSource.hpp>
#include <QObject>
#include <boost/compute/detail/lru_cache.hpp>

namespace bitcoin {
class CScript;
}

//==============================================================================

bitcoin::uint256 GetHash(const Wire::VerboseBlockHeader::Header& header);
bitcoin::uint256 GetHash(const Wire::MsgTx& tx);
Wire::VerboseBlockHeader::Header MakeHeader(int32_t version, std::string prevBlock,
    std::string merkleRoot, uint32_t timestamp, uint32_t bits, uint32_t nonce);

template <class T> std::string GetHashStr(const T& val)
{
    return GetHash(val).ToString();
}

Wire::MsgBlock MakeBlock(
    Wire::VerboseBlockHeader::Header header, std::vector<Wire::MsgTx> transactions = {});

//==============================================================================

class RegtestChain : public QObject {
    Q_OBJECT
public:
    explicit RegtestChain(
        AssetID assetID, const WalletAssetsModel& assetsModel, QObject* parent = nullptr);

    void load();

    size_t height() const;
    std::string bestBlockHash() const;
    Wire::MsgBlock blockAt(size_t index) const;
    Wire::MsgBlock blockByHash(BlockHash hash) const;
    int32_t indexOf(BlockHash hash) const;
    const Wire::VerboseBlockHeader::Header& tip() const;
    boost::optional<Wire::MsgTx> transactionByTxId(QString txid) const;

    std::vector<std::string> generateBlocks(bitcoin::CScript coinbase_script, int nGenerate);

    // using this method we can simulatre chain reorg.
    std::vector<std::string> reorganizeChain(
        bitcoin::CScript coinbase_script, size_t blockToDisconnect, size_t blocksToConnect);
    void processNewBlock(const Wire::MsgBlock& newBlock);

    std::vector<Wire::MsgTx>& mempool() const;
    const std::map<Wire::OutPoint, Wire::TxOut>& utxoSet() const;

    void lockOutpoint(const Wire::OutPoint& outpoint);
    void unlockOutpoint(const Wire::OutPoint& outpoint);

signals:
    void tipConnected(BlockHash newBlockTip, int newHeight);
    void tipDisconnected();

private:
    void disconnectTip();

private:
    AssetID _assetID;
    const WalletAssetsModel& _assetsModel;
    std::vector<Wire::MsgBlock> _blocks;
    mutable std::vector<Wire::MsgTx> _mempool;
    std::map<std::string, size_t> _blocksIndexByHash;
    std::map<Wire::OutPoint, Wire::TxOut> _unspentUTXOs;
    std::set<Wire::OutPoint> _lockedOutpoints;
    std::map<QString, BlockHash> _transactionIndex;

    using DisconnectedHeadersCache
        = boost::compute::detail::lru_cache<std::string, Wire::VerboseBlockHeader::Header>;
    mutable DisconnectedHeadersCache _disconnectedHeaders;
};

//==============================================================================

class RegtestDataSource : public AbstractChainDataSource, public UTXOSetDataSource {
    Q_OBJECT

public:
    RegtestDataSource(const WalletAssetsModel& assetsModel, QObject* parent = nullptr);

    // AbstractChainDataSource interface
public:
    Promise<std::vector<Wire::VerboseBlockHeader>> getBlockHeaders(
        AssetID assetID, BlockHash startingBlockHash) const override;
    Promise<Wire::VerboseBlockHeader> getBlockHeader(
        AssetID assetID, BlockHash hash) const override;
    Promise<BlockHash> getBlockHash(AssetID assetID, size_t blockHeight) const override;
    Promise<std::vector<unsigned char>> getBlock(AssetID assetID, BlockHash hash) const override;
    Promise<Wire::StrippedBlock> getLightWalletBlock(
        AssetID assetID, BlockHash hash, int64_t blockHeight) const override;
    Promise<std::vector<std::string>> getFilteredBlock(
        AssetID assetID, BlockHash hash) const override;
    Promise<Wire::EncodedBlockFilter> getBlockFilter(
        AssetID assetID, BlockHash hash) const override;
    Promise<QString> sendRawTransaction(AssetID assetID, QString serializedTx) const override;
    Promise<std::string> getRawTxByIndex(
        AssetID assetID, int64_t blockNum, uint32_t txIndex) const override;
    Promise<Wire::TxConfrimation> getRawTransaction(AssetID assetID, QString txid) const override;
    Promise<boost::optional<Wire::TxOut>> getTxOut(AssetID, Wire::OutPoint outpoint) const override;
    Promise<std::tuple<BlockHash, BlockHeight>> getBestBlockHash(AssetID assetID) const override;
    Promise<void> loadSecondLayerCache(
        AssetID assetID, uint32_t startBlock, Interrupt* onInterrupt) const override;
    Promise<void> freeSecondLayerCache(AssetID assetID) const override;
    Promise<int64_t> estimateNetworkFee(AssetID assetID, uint64_t blocks) const override;
    Promise<Wire::TxConfrimation> getSpendingDetails(
        AssetID assetID, Wire::OutPoint outpoint) const override;

    RegtestChain& chain(AssetID assetID) const;

    // UTXOSetDataSource interface
public:
    Promise<boost::optional<Wire::TxOut>> getUTXO(
        AssetID assetID, const Wire::OutPoint& outpoint) const override;
    Promise<std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>>> listUTXOs(
        AssetID assetID) const override;
    Promise<void> lockOutpoint(AssetID assetID, Wire::OutPoint outpoint) override;
    Promise<void> unlockOutpoint(AssetID assetID, Wire::OutPoint outpoint) override;
    Promise<void> load() override;

private:
    const WalletAssetsModel& _assetsModel;
    std::map<AssetID, RegtestChain*> _chains;
    std::map<AssetID, Wire::MsgBlock> _disconnectedBlocks;
};

//==============================================================================

class RegtestChainManager : public AbstractMutableChainManager {
public:
    explicit RegtestChainManager(
        RegtestDataSource& regtestDataSource, const WalletAssetsModel& assetsModel);
    // AbstractChainManager interface
public:
    Promise<void> loadChains(std::vector<AssetID> chains) override;
    Promise<void> loadFromBootstrap(QString bootstrapFile) override;

private:
    void connectTip(AssetID assetID, Wire::VerboseBlockHeader newTip);
    void disconnectTip(AssetID assetID);

private:
    RegtestDataSource& _dataSource;
};

//==============================================================================

class BlockAssembler {
public:
    explicit BlockAssembler(const RegtestChain& chain);
    Wire::MsgBlock CreateNewBlock(const bitcoin::CScript& coinbase_script);

private:
    const RegtestChain& _chain;
};

//==============================================================================

#endif // REGTESTCHAIN_HPP
