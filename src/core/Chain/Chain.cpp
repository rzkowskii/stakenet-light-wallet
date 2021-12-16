#include "Chain.hpp"
#include <Utils/Logging.hpp>
#include <crypto/common.h>

//==============================================================================

Chain::Chain(
    AssetID assetID, ConnectNewTip setNewTip, GetHeaderAtHeight getHeaderAt, QObject* parent)
    : QObject(parent)
    , _assetID(assetID)
    , _setNewTip(setNewTip)
    , _getHeaderAt(getHeaderAt)
{
}

//==============================================================================

Chain::~Chain() {}

//==============================================================================

AssetID Chain::assetID() const
{
    return _assetID;
}

//==============================================================================

BlockHash Chain::bestBlockHash() const
{
    return _bestBlockHash;
}

//==============================================================================

size_t Chain::getHeight() const
{
    return _chainHeight;
}

//==============================================================================

boost::optional<Wire::VerboseBlockHeader> Chain::headerAt(size_t height) const
{
    return _getHeaderAt(height);
}

//==============================================================================

void Chain::connectTip(Wire::VerboseBlockHeader newTip)
{
    LogCDebug(Chains) << "Connecting tip" << newTip.hash.c_str() << newTip.height;
    Q_ASSERT(_bestBlockHash.isEmpty() || _bestBlockHash.toStdString() == newTip.header.prevBlock);
    auto hash = QString::fromStdString(newTip.hash);
    if (hash != _bestBlockHash) {
        _bestBlockHash = hash;
        _chainHeight = newTip.height;
        _setNewTip(newTip);
        bestBlockHashChanged(_bestBlockHash);
    }
}

//==============================================================================

void Chain::setEthChain(QString hash, uint64_t height)
{
    LogCDebug(Chains) << "setEthChain" << hash << height;
    if (hash != _bestBlockHash) {
        _bestBlockHash = hash;
        _chainHeight = height;
        bestBlockHashChanged(_bestBlockHash);
    }
}

//==============================================================================

void Chain::disconnectTip()
{
    auto blockHeader = headerAt(_chainHeight - 1);
    Q_ASSERT(blockHeader);
    _bestBlockHash.clear();
    connectTip(blockHeader.get());
}

//==============================================================================

ChainView::ChainView(Chain* chain, bool compressEvents)
    : _chain(chain)
{
    if (compressEvents) {
        QTimer* eventDeliveryTimer = new QTimer(this);
        eventDeliveryTimer->setSingleShot(true);
        eventDeliveryTimer->setInterval(500);
        connect(eventDeliveryTimer, &QTimer::timeout, this, [this] {
            if (_chain) {
                bestBlockHashChanged(_chain->bestBlockHash());
            }
        });

        connect(_chain, &Chain::bestBlockHashChanged, eventDeliveryTimer, [eventDeliveryTimer] {
            if (eventDeliveryTimer && !eventDeliveryTimer->isActive()) {
                eventDeliveryTimer->start();
            }
        });
    } else {
        connect(_chain, &Chain::bestBlockHashChanged, this, &ChainView::bestBlockHashChanged);
    }
}

//==============================================================================

ChainView::~ChainView() {}

//==============================================================================

Promise<BlockHash> ChainView::bestBlockHash() const
{
    auto self = shared_from_this();
    return Promise<BlockHash>([=](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_chain, [=] {
            auto bbHash = self->_chain->bestBlockHash();
            resolver(bbHash);
        });
    });
}

//==============================================================================

Promise<size_t> ChainView::chainHeight() const
{
    auto self = shared_from_this();
    return Promise<size_t>([=](const auto& resolver, const auto&) {
        QMetaObject::invokeMethod(_chain, [=] { resolver(self->_chain->getHeight()); });
    });
}

//==============================================================================

AssetID ChainView::assetID() const
{
    return _chain->assetID();
}

//==============================================================================
