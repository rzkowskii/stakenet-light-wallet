#ifndef CHAINMANAGER_HPP
#define CHAINMANAGER_HPP

#include <Chain/AbstractChainManager.hpp>
#include <Chain/BlockIndex.hpp>

#include <boost/compute/detail/lru_cache.hpp>
#include <set>

namespace bitcoin {
class CBlockTreeDB;
}

class QTimer;

class ChainManager : public AbstractMutableChainManager {
public:
    ChainManager(const WalletAssetsModel& assetsModel, QString dataDir, QObject* parent = nullptr);

    Promise<void> loadChains(std::vector<AssetID> chains) override;
    Promise<void> loadFromBootstrap(QString bootstrapFile) override;

private slots:
    struct LastFlushInfo {
        BlockHash tipHash;
        size_t height;
    };

    void onStateFlushed();
    void onFlushError(QString error);

private:
    void flushStateToDisk();
    void startFlushTimer();
    void executeLoadChains(std::vector<AssetID> chains);
    void executeLoadBootstrap(QString path);

private:
    std::unique_ptr<bitcoin::CBlockTreeDB> _chainDB;
    std::map<AssetID, std::vector<Wire::VerboseBlockHeader>> _dirtyIndex;
    QTimer* _diskFlushTimer{ nullptr };
    std::string _dataDir;
};

#endif // CHAINMANAGER_HPP
