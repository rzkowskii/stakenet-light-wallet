#include "ApiClientNetworkingFactory.hpp"

#include <EthCore/Encodings.hpp>
#include <Networking/CMCRemotePriceProvider.hpp>
#include <Networking/NetworkConnectionState.hpp>
#include <Networking/RequestHandlerImpl.hpp>
#include <Networking/Web3Client.hpp>
#include <Networking/XSNBlockExplorerHttpClient.hpp>
#include <Networking/AccountExplorerHttpClient.hpp>
#include <QNetworkAccessManager>

static const unsigned BITCOINID = 0;
static const unsigned LITECOINID = 2;
static const unsigned STAKENETID = 384;
static const unsigned DASHCOINID = 5;
static const unsigned ETHEREUMCOINID = 60;
static const unsigned GROESTLCOINID = 17;
static const unsigned WRAPPEDETHER = 60001;
static const unsigned TETHERUSD = 60002;
static const unsigned USDCOINID = 60003;
static const unsigned ETHROPSTEN = 9999;
static const unsigned WRAPPEDETHROPSTEN = 9999001;
static const unsigned ETHRINKEBY = 88;

//==============================================================================

ApiClientNetworkingFactory::ApiClientNetworkingFactory(QObject* parent)
    : AbstractNetworkingFactory(parent)
    , _networkAccessManager(new QNetworkAccessManager(this))
{
}

//==============================================================================

ApiClientNetworkingFactory::~ApiClientNetworkingFactory() {}

//==============================================================================

AbstractNetworkingFactory::BlockExplorerHttpClient
ApiClientNetworkingFactory::createBlockExplorerClient(AssetID assetID)
{
    auto helper = [](AssetID assetID) {
        switch (assetID) {
        case LITECOINID:
            return QString("ltc");
        case STAKENETID:
            return QString("xsn");
        case BITCOINID:
            return QString("btc");
        case DASHCOINID:
            return QString("dash");
        case GROESTLCOINID:
            return QString("grs");
        case ETHEREUMCOINID:
            return QString("eth");
        case WRAPPEDETHER:
            return QString("weth");
        case TETHERUSD:
            return QString("usdt");
        case USDCOINID:
            return QString("usdc");
        case ETHROPSTEN:
            return QString("eth-ropsten"); // TODO: change to right name
        case WRAPPEDETHROPSTEN:
            return QString("weth-ropsten"); // TODO: change to right name
        case ETHRINKEBY:
            return QString("eth-rinkeby"); // TODO: change to right name

        default:
            Q_ASSERT_X(false, "create", "Unsupported asset id for block explorer");
        }

        return QString();
    };

    RequestHandlerImpl::Domains domains;
    auto assetSuffix = helper(assetID);
    domains.normal = QString("https://xsnexplorer.io/api/%1").arg(assetSuffix);
    domains.tor = QString("https://lightningstake.com/api/%1").arg(assetSuffix);

    std::unique_ptr<RequestHandlerImpl> requestHandler(new RequestHandlerImpl(
        _networkAccessManager, NetworkConnectionState::Singleton(), domains));
    BlockExplorerHttpClient client(new XSNBlockExplorerHttpClient(std::move(requestHandler)));
    client->moveToThread(thread());
    //    this produces assert on mingw, commenting it out is a bad idea, but still effect will be
    //    the same because networking factory gets deleted in the end of application.
    //    client->setParent(this);
    return client;
}

//==============================================================================

AbstractNetworkingFactory::RemotePriceProviderPtr
ApiClientNetworkingFactory::createRemotePriceProvider()
{
    RequestHandlerImpl::Domains domains;
    domains.normal = domains.tor = QString("https://xsnexplorer.io/api");
    std::unique_ptr<RequestHandlerImpl> requestHandler(
        new RequestHandlerImpl(_networkAccessManager, nullptr, domains));
    RemotePriceProviderPtr client(new CMCRemotePriceProvider(std::move(requestHandler)));
    client->moveToThread(thread());
    return client;
}

//==============================================================================

AbstractNetworkingFactory::AbstractWeb3ClientPtr ApiClientNetworkingFactory::createWeb3Client(
    uint32_t chainId)
{
    AbstractWeb3ClientPtr client(
        new Web3Client(QUrl(QString("wss://%1.infura.io/ws/v3/12535e2cda8c485eae0d513904a3723a")
                                .arg(eth::ConvertChainId(chainId)))));
    // TODO: jcon has broken timer, fix is before moving to thread
//    client->moveToThread(thread());
    client->open();
    return client;
}

//==============================================================================

AbstractNetworkingFactory::AccountExplorerHttpClientPtr ApiClientNetworkingFactory::createAccountExplorerClient(AssetID assetID)
{
    RequestHandlerImpl::Domains domains;
    domains.normal = QString("https://xsnexplorer.io/api/eth");
    domains.tor = domains.normal;//QString("https://lightningstake.com/api/%1").arg(assetSuffix); //TODO: change when it will be created

    std::unique_ptr<RequestHandlerImpl> requestHandler(new RequestHandlerImpl(
        _networkAccessManager, NetworkConnectionState::Singleton(), domains));
    AccountExplorerHttpClientPtr client(new AccountExplorerHttpClient(std::move(requestHandler)));
    client->moveToThread(thread());

    return client;
}

//==============================================================================
