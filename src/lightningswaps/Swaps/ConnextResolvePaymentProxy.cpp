#include "ConnextResolvePaymentProxy.hpp"

namespace swaps {

//==============================================================================

ConnextResolvePaymentProxy::ConnextResolvePaymentProxy(std::string rHash, QObject* parent)
    : QObject(parent)
    , _rHash(rHash)
{
}

//==============================================================================

void ConnextResolvePaymentProxy::onResolveRequest(ResolvedTransferResponse details)
{
    if (details.lockHash == _rHash) {
        emit resolved(details.rPreimage);
    }
}

//==============================================================================
}
