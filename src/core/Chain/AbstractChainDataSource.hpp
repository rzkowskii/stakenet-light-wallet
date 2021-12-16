#ifndef ABSTRACTCHAINDATASOURCE_HPP
#define ABSTRACTCHAINDATASOURCE_HPP

#include <Chain/BlockHeader.hpp>
#include <QObject>
#include <Utils/Utils.hpp>

class AbstractChainDataSource : public QObject {
    Q_OBJECT
public:
    using Interrupt = std::function<void(void)>;

    explicit AbstractChainDataSource(QObject* parent = nullptr);
    ~AbstractChainDataSource() override;

    virtual Promise<std::vector<Wire::VerboseBlockHeader>> getBlockHeaders(
        AssetID assetID, BlockHash startingBlockHash) const = 0;
    virtual Promise<Wire::VerboseBlockHeader> getBlockHeader(
        AssetID assetID, BlockHash hash) const = 0;
    virtual Promise<BlockHash> getBlockHash(AssetID assetID, size_t blockHeight) const = 0;
    virtual Promise<std::vector<unsigned char>> getBlock(AssetID assetID, BlockHash hash) const = 0;
    virtual Promise<Wire::StrippedBlock> getLightWalletBlock(
        AssetID assetID, BlockHash hash, int64_t blockHeight) const = 0;
    virtual Promise<std::tuple<BlockHash, BlockHeight>> getBestBlockHash(AssetID assetID) const = 0;
    virtual Promise<std::vector<std::string>> getFilteredBlock(
        AssetID assetID, BlockHash hash) const = 0;
    virtual Promise<Wire::EncodedBlockFilter> getBlockFilter(
        AssetID assetID, BlockHash hash) const = 0;
    virtual Promise<Wire::TxConfrimation> getRawTransaction(
        AssetID assetID, QString txid) const = 0;
    virtual Promise<std::string> getRawTxByIndex(
        AssetID assetID, int64_t blockNum, uint32_t txIndex) const = 0;
    virtual Promise<boost::optional<Wire::TxOut>> getTxOut(
        AssetID, Wire::OutPoint outpoint) const = 0;
    virtual Promise<QString> sendRawTransaction(AssetID assetID, QString serializedTx) const = 0;
    virtual Promise<void> loadSecondLayerCache(
        AssetID assetID, uint32_t startBlock, Interrupt* onInterrupt = nullptr) const = 0;
    virtual Promise<void> freeSecondLayerCache(AssetID assetID) const = 0;
    virtual Promise<int64_t> estimateNetworkFee(AssetID assetID, uint64_t blocks) const = 0;
    virtual Promise<Wire::TxConfrimation> getSpendingDetails(
        AssetID assetID, Wire::OutPoint outpoint) const = 0;
};

#endif // ABSTRACTCHAINDATASOURCE_HPP
