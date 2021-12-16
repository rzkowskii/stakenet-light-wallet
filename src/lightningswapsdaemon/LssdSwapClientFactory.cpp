#include "LssdSwapClientFactory.hpp"
#include <LndTools/ConnextHttpClient.hpp>
#include <LndTools/LndGrpcClient.hpp>
#include <Swaps/ConnextHttpResolveService.hpp>
#include <Swaps/ConnextSwapClient.hpp>
#include <Swaps/LndSwapClient.hpp>

#include <QNetworkAccessManager>

//==============================================================================

LssdSwapClientFactory::LssdSwapClientFactory()
    : _accessManager{ new QNetworkAccessManager(this) }
{
}

//==============================================================================

void LssdSwapClientFactory::registerClientConf(std::string currency, LndClientConfiguration conf)
{
    if (_grpcClients.count(currency) == 0) {
        auto certProvider = [tlsCert = conf.tlsCert] { return tlsCert; };
        LndGrpcClient::MacaroonProvider macaroonProvider;
        if (!conf.macaroon.empty()) {
            macaroonProvider = [macaroon = conf.macaroon] { return macaroon; };
        }
        _grpcClients.emplace(currency,
            std::unique_ptr<LndGrpcClient>(new LndGrpcClient(
                QString::fromStdString(conf.channel), certProvider, macaroonProvider)));
    }

    _grpcClients.at(currency)->connect();
}

//==============================================================================

void LssdSwapClientFactory::registerConnextConf(
    std::string currency, ConnextClientConfiguration conf)
{
    _connextCurrencies[currency] = conf.tokenAddress;

    if (_connextHttpClient) {
        return;
    }

    auto resolverHost
        = QString::fromStdString(conf.resolverAddress).split(":").first().toStdString();
    auto resolverPort = QString::fromStdString(conf.resolverAddress).split(":")[1].toUInt();

    initializeConnextResolveService(resolverHost, resolverPort);

    auto comps = QString::fromStdString(conf.channel).split(':');

    if (comps.size() != 2) {
        throw std::runtime_error("Invalid connext channel");
    }

    RequestHandlerImpl::Domains domains;
    domains.normal = domains.tor
        = QString("http://%1:%2").arg(comps.at(0)).arg(comps.at(1).toInt());
    std::unique_ptr<RequestHandlerImpl> requestHandler(
        new RequestHandlerImpl(_accessManager, nullptr, domains));
    _connextHttpClient = new ConnextHttpClient(std::move(requestHandler), this);
}

//==============================================================================

std::unique_ptr<swaps::AbstractSwapLndClient> LssdSwapClientFactory::createLndSwapClient(
    std::string currency)
{
    if (_grpcClients.count(currency) == 0) {
        return {};
    }

    return std::make_unique<swaps::LndSwapClient>(_grpcClients.at(currency).get(), currency);
}

//==============================================================================

std::unique_ptr<swaps::AbstractSwapConnextClient> LssdSwapClientFactory::createConnextSwapClient(
    std::string currency)
{
    if (_connextCurrencies.count(currency) == 0) {
        return {};
    }

    return std::make_unique<swaps::ConnextSwapClient>(
        _connextHttpClient, _connextCurrencies.at(currency));
}

//==============================================================================

std::unique_ptr<swaps::AbstractConnextResolveService>
LssdSwapClientFactory::createConnextResolveService()
{
    return std::make_unique<ConnextResolveServiceProxy>(_connextResolver);
}

//==============================================================================

void LssdSwapClientFactory::initializeConnextResolveService(std::string host, unsigned short port)
{
    if (_connextResolver) {
        return;
    }
    _connextResolver = new swaps::ConnextHttpResolveService{ this };
    _connextResolver->start(host, port);
}

//==============================================================================

ConnextResolveServiceProxy::ConnextResolveServiceProxy(
    swaps::AbstractConnextResolveService* resolver, QObject* parent)
    : swaps::AbstractConnextResolveService(parent)
{
    connect(resolver, &swaps::AbstractConnextResolveService::receivedResolveRequest, this,
        &swaps::AbstractConnextResolveService::receivedResolveRequest);
    connect(resolver, &swaps::AbstractConnextResolveService::receivedResolvedTransfer, this,
        &swaps::AbstractConnextResolveService::receivedResolvedTransfer);
}

//==============================================================================
