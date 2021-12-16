#include "SwapClientPool.hpp"
#include <LndTools/LndGrpcClient.hpp>
#include <SwapService.hpp>
#include <Swaps/AbstractSwapClientFactory.hpp>
#include <Swaps/ConnextHttpResolveService.hpp>
#include <Swaps/ConnextSwapClient.hpp>
#include <Swaps/LndSwapClient.hpp>

namespace swaps {

//==============================================================================

SwapClientPool::SwapClientPool(AbstractSwapClientFactory& clientFactory, QObject* parent)
    : AbstractSwapClientPool(clientFactory, parent)
{
}

//==============================================================================

SwapClientPool::~SwapClientPool() {}

//==============================================================================

AbstractSwapClient* SwapClientPool::getClient(std::string currency) const
{
    if (_connextClients.count(currency) > 0) {
        return _connextClients.at(currency).get();
    }
    return _lndClients.at(currency).get();
}

//==============================================================================

AbstractSwapLndClient* SwapClientPool::getLndClient(std::string currency) const
{
    if (_lndClients.count(currency) == 0) {
        LogCCInfo(Swaps) << "Currency : " << QString::fromStdString(currency);
        LogCCInfo(Swaps) << "Lnd clients : ";
        for (auto&& it : _lndClients) {
            LogCCInfo(Swaps) << it.first.data();
        }
    }
    return _lndClients.at(currency).get();
}

//==============================================================================

bool SwapClientPool::hasClient(std::string currency) const
{
    if (_lndClients.count(currency) > 0 || _connextClients.count(currency) > 0)
        return true;
    else
        return false;
}

//==============================================================================

bool SwapClientPool::hasLndClient(std::string currency) const
{
    return _lndClients.count(currency) > 0;
}

//==============================================================================

std::vector<std::string> SwapClientPool::activeClients() const
{
    std::vector<std::string> clients;
    for (auto&& it : _lndClients) {
        clients.emplace_back(it.first);
    }

    for (auto&& it : _connextClients) {
        clients.emplace_back(it.first);
    }

    return clients;
}

//==============================================================================

void SwapClientPool::addLndClient(std::string currency)
{
    if (_lndClients.count(currency) == 0) {
        QVariantMap payload;
        if (auto lndClient = _clientFactory.createLndSwapClient(currency)) {
            connect(lndClient.get(), &AbstractSwapClient::htlcAccepted, this,
                std::bind(&SwapClientPool::htlcAccepted, this, std::placeholders::_1,
                    std::placeholders::_2, currency, payload));
            _lndClients.emplace(currency, std::move(lndClient));
            clientAdded(currency);
        }
    }
}

//==============================================================================

void SwapClientPool::addConnextClient(std::string currency)
{
    if (!_connextResolver) {
        _connextResolver = _clientFactory.createConnextResolveService();
        connect(_connextResolver.get(), &swaps::ConnextHttpResolveService::receivedResolveRequest,
            this, &AbstractSwapClientPool::resolveRequestReady);
    }

    if (_connextClients.count(currency) == 0) {
        QVariantMap payload;
        if (auto connextClient = _clientFactory.createConnextSwapClient(currency)) {
            connect(connextClient.get(), &AbstractSwapClient::htlcAccepted, this,
                std::bind(&SwapClientPool::htlcAccepted, this, std::placeholders::_1,
                    std::placeholders::_2, currency, payload));
            connect(_connextResolver.get(), &ConnextHttpResolveService::receivedResolvedTransfer,
                connextClient.get(), &AbstractSwapConnextClient::onTransferResolved);
            _connextClients.emplace(currency, std::move(connextClient));
            clientAdded(currency);
        }
    }
}

//==============================================================================
}
