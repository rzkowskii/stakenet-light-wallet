#include "AbstractSwapPeer.hpp"

namespace swaps {

//==============================================================================

AbstractSwapPeer::AbstractSwapPeer(std::string nodePubKey, QObject* parent)
    : QObject(parent)
    , _nodePubKey(nodePubKey)
{
}

//==============================================================================

std::string AbstractSwapPeer::nodePubKey() const
{
    return _nodePubKey;
}

//==============================================================================
}
