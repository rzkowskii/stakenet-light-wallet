#include "AbstractSwapConnextClient.hpp"

namespace swaps {

//==============================================================================

AbstractSwapConnextClient::AbstractSwapConnextClient() {}

//==============================================================================

AbstractSwapConnextClient::~AbstractSwapConnextClient() {}

//==============================================================================

AbstractConnextResolveService::AbstractConnextResolveService(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================
}
