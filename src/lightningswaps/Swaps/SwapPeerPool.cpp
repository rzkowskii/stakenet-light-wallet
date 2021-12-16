#include "SwapPeerPool.hpp"

namespace swaps {

SwapPeerPool::SwapPeerPool(QObject* parent)
    : AbstractSwapPeerPool(parent)
{
}

AbstractSwapPeer* swaps::SwapPeerPool::peerByPubKey(std::string pubKey) const
{
    return nullptr;
}
}
