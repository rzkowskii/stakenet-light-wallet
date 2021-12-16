#ifndef CHAIN_HPP
#define CHAIN_HPP

#include <Chain/BlockIndex.hpp>
#include <QObject>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>
#include <optional.h>
#include <unordered_map>
#include <vector>

//==============================================================================

class Chain : public QObject {
    Q_OBJECT
public:
    using ConnectNewTip = std::function<void(Wire::VerboseBlockHeader)>;
    using GetHeaderAtHeight = std::function<boost::optional<Wire::VerboseBlockHeader>(size_t)>;

    explicit Chain(AssetID assetID, ConnectNewTip setNewTip, GetHeaderAtHeight getHeaderAt,
        QObject* parent = nullptr);

    ~Chain();

    AssetID assetID() const;

    BlockHash bestBlockHash() const;
    size_t getHeight() const;

    boost::optional<Wire::VerboseBlockHeader> headerAt(size_t height) const;

    /** Set/initialize a chain with a given tip. */
    void connectTip(Wire::VerboseBlockHeader newTip);
    void setEthChain(QString hash, uint64_t height);
    void disconnectTip();

signals:
    void bestBlockHashChanged(BlockHash blockHash);

private:
    AssetID _assetID;
    BlockHash _bestBlockHash;
    size_t _chainHeight{ 0 };
    ConnectNewTip _setNewTip;
    GetHeaderAtHeight _getHeaderAt;
};

//==============================================================================

class ChainView : public QObject, public std::enable_shared_from_this<ChainView> {
    Q_OBJECT
public:
    explicit ChainView(Chain* chain, bool compressEvents);
    ~ChainView();
    Promise<BlockHash> bestBlockHash() const;
    Promise<size_t> chainHeight() const;
    AssetID assetID() const;

signals:
    void bestBlockHashChanged(BlockHash newTip);

private:
    QTimer* _tipChangedTimer{ nullptr };
    QPointer<Chain> _chain;
};

//==============================================================================

#endif // CHAIN_HPP
