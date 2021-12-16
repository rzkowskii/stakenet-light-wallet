#ifndef ABSTRACTCHAINMANAGER_HPP
#define ABSTRACTCHAINMANAGER_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>

class WalletAssetsModel;
class Chain;
class ChainView;

namespace Utils {
class WorkerThread;
}

class AbstractChainManager : public QObject {
    Q_OBJECT
public:
    explicit AbstractChainManager(const WalletAssetsModel& assetsModel, QObject* parent = nullptr);
    virtual ~AbstractChainManager();

    enum class ChainViewUpdatePolicy {
        CompressedEvents, // compress chain updates
        DecomporessedEvents // deliver every chain update(be careful with UI thread)
    };

    Promise<std::shared_ptr<ChainView>> getChainView(
        AssetID id, ChainViewUpdatePolicy policy) const;
    virtual Promise<void> loadChains(std::vector<AssetID> chains) = 0;
    virtual Promise<void> loadFromBootstrap(QString bootstrapFile) = 0;

signals:
    void chainsLoaded(std::vector<AssetID> chains);

protected:
    QObject* _executionContext{ nullptr };
    const WalletAssetsModel& _assetsModel;
    std::map<AssetID, std::unique_ptr<Chain>> _chains;
};

/*!
 * \brief The AbstractMutableChainManager class allows to access chains for modifications,
 * like setting new bestblock hash, this class is not thread-safe, you must understand what are you
 * doing to use it.
 */
class AbstractMutableChainManager : public AbstractChainManager {
public:
    AbstractMutableChainManager(const WalletAssetsModel& assetsModel, QObject* parent = nullptr);
    bool hasChain(AssetID assetId) const;
    Chain& chainById(AssetID assetId) const;
    std::vector<AssetID> chains() const;
};

#endif // ABSTRACTCHAINMANAGER_HPP
