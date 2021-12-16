#include "AbstractSwapClientPool.hpp"
#include <Swaps/AbstractSwapClient.hpp>

namespace swaps {

//==============================================================================

AbstractSwapClientPool::AbstractSwapClientPool(
    AbstractSwapClientFactory& clientFactory, QObject* parent)
    : QObject(parent)
    , _clientFactory(clientFactory)
{
}

//==============================================================================

AbstractSwapClientPool::~AbstractSwapClientPool() {}

//==============================================================================

AbstractLndSwapClientPool::~AbstractLndSwapClientPool() {}

//==============================================================================
}
