#ifndef CHAINSYNCHELPER_HPP
#define CHAINSYNCHELPER_HPP

#include <Chain/BlockHeader.hpp>
#include <Data/CoinAsset.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Networking/AbstractBlockExplorerHttpClient.hpp>
#include <QObject>
#include <QPointer>
#include <Utils/Utils.hpp>

class AbstractBlockExplorerHttpClient;

class ChainSyncHelper : public QObject {
    Q_OBJECT
public:
    explicit ChainSyncHelper(QPointer<AbstractBlockExplorerHttpClient> blockExplorerHttpClient,
        CoinAsset asset, QObject* parent = nullptr);
    ~ChainSyncHelper() override;

    void getBestBlockHash();
    AssetID assetId() const;

    Promise<std::vector<Wire::VerboseBlockHeader>> getLastHeaders(QString lastHeaderId);
    Promise<Wire::StrippedBlock> getLightWalletBlock(QString hash, int64_t blockHeight);
    Promise<BlockHash> getBlockHash(unsigned int blockIndex) const;

signals:
    void bestBlockSynced(BlockHash bestBlockHash, BlockHeight height);
    void bestBlockHashFailed(QString errorStr);

private slots:
    void onGetBestBlockHashResponseFinished(QByteArray response);
    void onGetBestBlockHashFailed(const NetworkUtils::ApiErrorException& error);

private:
    QPointer<AbstractBlockExplorerHttpClient> _blockExplorerHttpClient;
    CoinAsset _asset;
};

#endif // CHAINSYNCHELPER_HPP
