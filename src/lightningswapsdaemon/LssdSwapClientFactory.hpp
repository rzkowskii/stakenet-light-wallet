#ifndef LSSDSWAPCLIENTFACTORY_HPP
#define LSSDSWAPCLIENTFACTORY_HPP

#include <LndTools/LndGrpcClient.hpp>
#include <Swaps/AbstractSwapClientFactory.hpp>
#include <Swaps/AbstractSwapConnextClient.hpp>

#include <QPointer>
#include <map>

class QNetworkAccessManager;
class ConnextHttpClient;

namespace swaps {
class ConnextHttpResolveService;
}

//==============================================================================

struct LndClientConfiguration {
    std::string channel;
    std::string tlsCert;
    std::string macaroon;
};

//==============================================================================

struct ConnextClientConfiguration {
    std::string channel;
    std::string tokenAddress;
    std::string resolverAddress;
};

//==============================================================================

class LssdSwapClientFactory : public QObject, public swaps::AbstractSwapClientFactory {
    Q_OBJECT
public:
    LssdSwapClientFactory();

    void registerClientConf(std::string currency, LndClientConfiguration conf);
    void registerConnextConf(std::string currency, ConnextClientConfiguration conf);
    std::unique_ptr<swaps::AbstractSwapLndClient> createLndSwapClient(
        std::string currency) override;
    std::unique_ptr<swaps::AbstractSwapConnextClient> createConnextSwapClient(
        std::string currency) override;
    std::unique_ptr<swaps::AbstractConnextResolveService> createConnextResolveService() override;

private:
    void initializeConnextResolveService(std::string host, unsigned short port);

private:
    std::map<std::string, std::unique_ptr<LndGrpcClient>> _grpcClients;
    std::map<std::string, std::string> _connextCurrencies; /* currency -> tokenAddress */
    QPointer<QNetworkAccessManager> _accessManager;
    QPointer<ConnextHttpClient> _connextHttpClient;
    QPointer<swaps::ConnextHttpResolveService> _connextResolver;
};

//==============================================================================

/*!
 * \brief The ConnextResolveServiceProxy class is a proxy which borrows
 * AbstractConnextResolveService and provides access to signals without affecting lifetime of
 * resolver which needs to be run as single instance, needed for using in factory.
 */
class ConnextResolveServiceProxy : public swaps::AbstractConnextResolveService {
public:
    explicit ConnextResolveServiceProxy(
        AbstractConnextResolveService* resolver, QObject* parent = nullptr);
};

//==============================================================================

#endif // LSSDSWAPCLIENTFACTORY_HPP
