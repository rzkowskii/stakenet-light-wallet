#include "tst_swaps.hpp"
#include <LndTools/ConnextHttpClient.hpp>
#include <LndTools/LndGrpcClient.hpp>
#include <LndTools/LndTypes.hpp>
#include <Networking/OrderbookConnection.hpp>
#include <Networking/RequestHandlerImpl.hpp>
#include <Orderbook/AbstractOrderbookClient.hpp>
#include <Orderbook/OrderbookApiClient.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/OrderbookEventDispatcher.hpp>
#include <Orderbook/Types.hpp>
#include <SwapService.hpp>
#include <Swaps/AbstractSwapClientFactory.hpp>
#include <Swaps/AbstractSwapConnextClient.hpp>
#include <Swaps/AbstractSwapRepository.hpp>
#include <Swaps/ConnextHttpResolveService.hpp>
#include <Swaps/ConnextSwapClient.hpp>
#include <Swaps/LndSwapClient.hpp>
#include <Swaps/SwapClientPool.hpp>

#include <QNetworkAccessManager>
#include <QSignalSpy>
#include <QUuid>
#include <QtNetwork>
#include <cmath>

//==============================================================================

static std::tuple<std::unique_ptr<LndGrpcClient>, std::unique_ptr<swaps::LndSwapClient>>
CreateAndConnectLndClient(QString channel, QString tlsCertPath)
{
    auto grpcClient = std::make_unique<LndGrpcClient>(
        channel, [tlsCertPath] { return Utils::ReadCert(tlsCertPath); });
    grpcClient->connect();

    QObject dummy;
    QSignalSpy dummySpy(&dummy, &QObject::destroyed);
    dummySpy.wait(100);

    auto client = std::make_unique<swaps::LndSwapClient>(grpcClient.get(), "BTC");
    return std::make_tuple(std::move(grpcClient), std::move(client));
}

//==============================================================================

template <class T> static orderbook::OwnOrder ConvertToOrderbookOrder(const T& order)
{
    return orderbook::OwnOrder{
        orderbook::OwnLimitOrder{
            orderbook::LimitOrder{
                orderbook::MarketOrder{ order.quantity, order.pairId, order.isBuy }, order.price },
            orderbook::Local{ {}, 0 }, {} },
        orderbook::Stamp{
            orderbook::OrderIdentifier{ order.id }, order.createdAt, order.initialQuantity },
        false
    };
}

//==============================================================================

struct MockedSwapClient : AbstractSwapLndClient {
    MockedSwapClient(std::string curr, std::string alias)
        : currency(curr)
        , alias(alias)
    {
    }

    Promise<std::string> sendPayment(SwapDeal deal) override
    {
        return connectedClient->destination().then([deal, this](std::string dest) {
            if ((deal.role == SwapRole::Taker && dest == deal.destination)
                || (deal.role == SwapRole::Maker && dest == deal.takerPubKey.get())) {
                auto amount = deal.role == SwapRole::Maker ? deal.takerAmount : deal.makerAmount;
                LogDebug() << currency.c_str() << alias.c_str()
                           << "sending payment to:" << dest.c_str() << "amount:" << amount
                           << (deal.role == SwapRole::Maker ? deal.takerCurrency
                                                            : deal.makerCurrency)
                                  .c_str()
                           << "role:" << (deal.role == SwapRole::Maker ? "Maker" : "Taker");
                return connectedClient->handlePayment(deal.rHash, amount);
            } else {
                return Promise<std::string>::reject(QtPromise::QPromiseUndefinedException{});
            }
        });
    }

    Promise<lnrpc::Payment> sendPayment(
        std::string paymentRequest, lndtypes::LightningPaymentReason reason) override
    {
        return Promise<lnrpc::Payment>::reject(QtPromise::QPromiseUndefinedException{});
    }

    Promise<void> verifyConnection() override { return QtPromise::resolve(); }

    Promise<lnrpc::AddInvoiceResponse> addInvoice(
        int64_t units, lndtypes::LightingInvoiceReason reason) override
    {
        return Promise<lnrpc::AddInvoiceResponse>::reject(QtPromise::QPromiseUndefinedException{});
    }

    Promise<std::string> addHodlInvoice(
        std::string rHash, swaps::u256 units, uint32_t cltvExpiry) override
    {
        return QtPromise::resolve().delay(500).then([=] {
            invoices.emplace(rHash, Invoice{ units, cltvExpiry });
            return std::string{};
        });
    }

    Promise<void> settleInvoice(
        std::string rHash, std::string rPreimage, QVariantMap payload) override
    {
        return QtPromise::resolve().delay(500).then([=] { preimages[rHash] = rPreimage; });
    }

    Promise<void> removeInvoice(std::string rHash) override
    {
        return QtPromise::resolve().delay(10).then(
            [=] { return Promise<void>::reject(QtPromise::QPromiseUndefinedException()); });
    }

    Promise<uint32_t> getHeight() override
    {
        return QtPromise::resolve().delay(100).then([=] { return height; });
    }

    Promise<LightningPayRequest> decodePayRequest(std::string paymentRequest) override
    {
        return Promise<LightningPayRequest>::resolve({});
    }

    uint32_t finalLock() const override
    {
        return static_cast<uint32_t>(std::round(400 / minutesPerBlock()));
    }
    ClientType type() const override { return ClientType::Lnd; }

    Promise<Routes> getRoutes(swaps::u256 units, std::string destination, std::string currency,
        uint32_t finalCltvDelta) override
    {
        return QtPromise::resolve().delay(10).then([=] { return routes; });
    }

    virtual double minutesPerBlock() const override
    {
        return MINUTES_PER_BLOCK_BY_CURRENCY.at(currency);
    }
    uint32_t lockBuffer() const override
    {
        return static_cast<uint32_t>(std::round(LOCK_BUFFER_HOURS * 60 / minutesPerBlock()));
    }
    bool isConnected() const override { return connected; }

    Promise<std::vector<LndChannel>> activeChannels() const override
    {
        return QtPromise::resolve(std::vector<LndChannel>{});
    }

    Promise<std::string> destination() const override
    {
        return QtPromise::resolve("lnd/" + currency + "/" + alias);
    }

    static Route GenerateRoute(std::string peerPubKey, uint64_t channelId, int height)
    {
        Route route;
        Route::Hop hop;
        hop.pub_key = peerPubKey;
        hop.chan_id = channelId;
        route.hops.emplace_back(hop);
        route.total_time_lock = height + 25;
        return route;
    }

    Promise<std::string> handlePayment(std::string rHash, int64_t amount)
    {
        return Promise<std::string>([=](const auto& resolve, const auto& reject) {
            this->htlcAccepted(rHash, amount);
            QtPromise::resolve().delay(2000).then([=] {
                if (preimages.count(rHash) > 0) {
                    resolve(preimages.at(rHash));
                } else {
                    reject();
                }
            });
        });
    }

    MockedSwapClient* connectedClient;
    std::string currency;
    std::string alias;
    Routes routes;
    using Invoice = std::tuple<swaps::u256, uint32_t>;
    std::map<std::string, Invoice> invoices;
    std::map<std::string, std::string> preimages;
    uint32_t height{ 10 };
    bool connected{ true };
};

//==============================================================================

struct MockedPeer : AbstractSwapPeer {
    // AbstractSwapPeer interface

    MockedPeer(std::string pubKey)
        : AbstractSwapPeer(pubKey)
    {
    }

    void pushPacket(std::vector<unsigned char> serialized)
    {
        QTimer::singleShot(0, this, [=] { packetReceived(serialized); });
    }

    void connectOther(MockedPeer* other) { _otherPeer = other; }

    Promise<void> sendPacket(packets::Packet packet) override
    {
        if (!_otherPeer) {
            return Promise<void>([](const auto&, const auto& reject) { reject(); });
        } else {
            return QtPromise::resolve().delay(500).then([=] {
                std::vector<unsigned char> blob(static_cast<size_t>(packet.ByteSize()));
                packet.SerializeToArray(blob.data(), blob.size());
                _otherPeer->pushPacket(blob);
            });
        }
    }

    MockedPeer* _otherPeer{ nullptr };
};

//==============================================================================

struct MockedPeersPool : AbstractSwapPeerPool {
    void addPeer(std::unique_ptr<MockedPeer> peer)
    {
        connect(peer.get(), &AbstractSwapPeer::packetReceived, this,
            &MockedPeersPool::onPacketRecevied);
        _peers[peer->nodePubKey()] = std::move(peer);
    }

    AbstractSwapPeer* peerByPubKey(std::string pubKey) const override
    {
        return _peers.count(pubKey) > 0 ? _peers.at(pubKey).get() : nullptr;
    }

    void onPacketRecevied(std::vector<unsigned char> serialized)
    {
        auto packet = UnserializePacket(serialized);
        auto peer = qobject_cast<swaps::AbstractSwapPeer*>(sender());
        switch (packet.swap_case()) {
        case packets::Packet::SwapCase::kReq:
            swapRequestReceived(packet.req(), peer);
            break;
        case packets::Packet::SwapCase::kAccepted:
            swapAcceptedReceived(packet.accepted(), peer);
            break;
        case packets::Packet::SwapCase::kComplete:
            swapCompleteReceived(packet.complete(), peer);
            break;
        case packets::Packet::SwapCase::kFail:
            swapFailReceived(packet.fail(), peer);
            break;
        case packets::Packet::SwapCase::kInvoiceExchange:
            swapInvoiceExchangeReceived(packet.invoice_exchange(), peer);
            break;
        case packets::Packet::SwapCase::kExchangeAck:
            swapInvoiceExchangeAckReceived(packet.exchange_ack(), peer);
            break;
        default:
            break;
        }
    }

    std::map<std::string, std::unique_ptr<MockedPeer>> _peers;
};

//==============================================================================

struct MockedSwapClientFactory : AbstractSwapClientFactory {
    // AbstractSwapClientFactory interface
public:
    std::unique_ptr<AbstractSwapLndClient> createLndSwapClient(std::string currency) override
    {
        return std::make_unique<MockedSwapClient>(currency, "mocked-from-factory");
    }
    std::unique_ptr<AbstractSwapConnextClient> createConnextSwapClient(
        std::string currency) override
    {
        return nullptr;
    }
    std::unique_ptr<AbstractConnextResolveService> createConnextResolveService() override
    {
        return nullptr;
    }
};

//==============================================================================

struct MockedClientsPool : AbstractSwapClientPool {
    using AbstractSwapClientPool::AbstractSwapClientPool;

    void addClient(std::string currency, std::unique_ptr<AbstractSwapClient> client)
    {
        QVariantMap payload;
        connect(client.get(), &AbstractSwapClient::htlcAccepted, this,
            [this, currency, payload](const auto rHash, const auto amount) {
                this->htlcAccepted(rHash, amount, currency, payload);
            });
        _swapClients.emplace(currency, std::move(client));
    }

    AbstractSwapClient* connextClient() const override { return _connextClient.get(); }

    AbstractSwapClient* getClient(std::string currency) const override
    {
        if (auto lndClient
            = _swapClients.count(currency) > 0 ? _swapClients.at(currency).get() : nullptr) {
            return lndClient;
        }

        return _connextTokens.count(currency) > 0 ? connextClient() : nullptr;
    }

    std::vector<std::string> activeClients() const override
    {
        std::vector<std::string> result;
        for (auto&& it : _swapClients) {
            result.emplace_back(it.first);
        }

        std::copy(std::begin(_connextTokens), std::end(_connextTokens), std::back_inserter(result));

        return result;
    }

    void initConnextTokens(std::string token)
    {
        _connextTokens.clear();
        if (auto connext = dynamic_cast<ConnextSwapClient*>(_connextClient.get())) {
            connext->init(token);
            _connextTokens.insert("ETH");
        }
    }

    void startConnextResolver(std::string address, int port)
    {
        _connextResolver = new ConnextHttpResolveService{ this };
        connect(_connextResolver, &swaps::ConnextHttpResolveService::receivedResolveRequest, this,
            &swaps::SwapClientPool::resolveRequestReady);

        _connextResolver->start(address, port);
    }

    std::unordered_map<std::string, std::unique_ptr<AbstractSwapClient>> _swapClients;
    std::set<std::string> _connextTokens;
    std::unique_ptr<AbstractSwapConnextClient> _connextClient;
    QPointer<swaps::ConnextHttpResolveService> _connextResolver;
};

//==============================================================================

struct MockedOrderBook : orderbook::AbstractOrderbookClient {

    // AbstractOrderBookClient interface
public:
    orderbook::OwnOrder getOwnOrder(std::string pairId, std::string orderId) const override
    {
        return _orders.at(pairId).at(orderId);
    }
    boost::optional<orderbook::OwnOrder> tryGetOwnOrder(
        std::string pairId, std::string orderId) const
    {
        return _orders.count(pairId) > 0 && _orders.at(pairId).count(orderId) > 0
            ? boost::make_optional(_orders.at(pairId).at(orderId))
            : boost::none;
    }

    using OwnOrders = std::map<std::string, orderbook::OwnOrder>;
    std::map<std::string, OwnOrders> _orders;
};

struct MockedRepository : public swaps::AbstractSwapRepository {

    std::map<std::string, SwapDeal> _deals;

    // AbstractSwapRepository interface
protected:
    Deals executeLoadAll() override { return {}; }

    void executeSaveDeal(SwapDeal deal) override { _deals[deal.rHash] = deal; }
};

//==============================================================================

OrderbookClientTests::OrderbookClientTests()
{
    qRegisterMetaType<std::string>("std::string");
}

//==============================================================================

void OrderbookClientTests::startAndGetPairId(
    orderbook::OrderbookClient& orderbook, std::string& pairId)
{
    QSignalSpy startSpy(&orderbook, &orderbook::OrderbookClient::stateChanged);
    orderbook.start();
    ASSERT_TRUE(startSpy.count() > 0 || startSpy.wait());

    orderbook.tradingPairs()
        .then([&pairId](orderbook::OrderbookClient::TradingPairInfo info) {
            auto pairs = std::get<0>(info);
            ASSERT_FALSE(pairs.empty());
            pairId = pairs.front().pairId;
        })
        .wait();

    ASSERT_FALSE(orderbook.subscribe(pairId).wait().isRejected());
    ASSERT_FALSE(pairId.empty());
}

//==============================================================================

void OrderbookClientTests::SetUp()
{
    std::map<QString, QString> orderbookParams{
        { orderbook::OrderbookParamsKeys::HEADER_API_VERSION, QString::number(1) }
    };

    {
        auto apiClient = std::make_unique<orderbook::OrderbookApiClient>(
            SwapService::Config::StagingOrderbookUrl(), orderbookParams);

        makerOrderbook.reset(new orderbook::OrderbookClient(std::move(apiClient)));
    }

    {
        auto apiClient = std::make_unique<orderbook::OrderbookApiClient>(
            SwapService::Config::StagingOrderbookUrl(), orderbookParams);

        takerOrderbook.reset(new orderbook::OrderbookClient(std::move(apiClient)));
    }
}

//==============================================================================

void OrderbookClientTests::TearDown()
{
    makerOrderbook.reset();
    takerOrderbook.reset();
    pairId.clear();
}

//==============================================================================

SwapTests::SwapTests()
{
    qRegisterMetaType<swaps::SwapSuccess>("swaps::SwapSuccess");
}

//==============================================================================

std::tuple<OwnOrder, PeerOrder> SwapTests::CreateOrderPair(std::string pairId, bool isBuy,
    int64_t price, std::pair<int64_t, int64_t> initialQuantity, std::string peerPubKey)
{
    OwnOrder ownOrder;
    PeerOrder peerOrder;

    ownOrder.id = QUuid::createUuid().toString().toStdString();
    ownOrder.pairId = pairId;
    ownOrder.isBuy = isBuy;
    ownOrder.quantity = initialQuantity.first;
    ownOrder.createdAt = QDateTime::currentMSecsSinceEpoch();
    ownOrder.price = price;
    ownOrder.localId = ownOrder.id;

    peerOrder.id = QUuid::createUuid().toString().toStdString();
    peerOrder.pairId = pairId;
    peerOrder.isBuy = !ownOrder.isBuy;
    peerOrder.quantity = initialQuantity.second;
    peerOrder.createdAt = QDateTime::currentMSecsSinceEpoch();
    peerOrder.price = price;
    peerOrder.peerPubKey = peerPubKey;

    return std::make_tuple(ownOrder, peerOrder);
}

//==============================================================================

std::tuple<OwnOrder, PeerOrder> SwapTests::createDefaultTestOrder()
{
    Q_ASSERT(maker.peer);
    return CreateOrderPair(
        "BTC_LTC", // buy/sell, if I want to buy BTC and sell LTC then (BTC/LTC, isBuy = false)
        false, // we are selling and taking order
        0.00097200 * COIN,
        std::make_pair(0.16757115 * COIN, 0.16757115 * COIN), // own and peer order quantity
        maker.peer->nodePubKey());
}

//==============================================================================

void SwapTests::SetUp()
{
    maker.clientFactory.reset(new MockedSwapClientFactory);
    taker.clientFactory.reset(new MockedSwapClientFactory);

    maker.repository.reset(new MockedRepository);
    taker.repository.reset(new MockedRepository);

    std::unique_ptr<MockedPeer> takerPeer(
        new MockedPeer("vector82HffkBSTeSuvuEApLqLvY4wStnQYk7ysxZfYR2MmkwEe4XBbt"));
    std::unique_ptr<MockedPeer> makerPeer(
        new MockedPeer("vector6RJffyPFY7D4pAkyooRxgXVyocxYkRUW3NFQVznHazn98vcadh"));

    takerPeer->connectOther(makerPeer.get());
    makerPeer->connectOther(takerPeer.get());

    maker.orderbook.reset(new MockedOrderBook());
    taker.orderbook.reset(new MockedOrderBook());

    maker.peers.reset(new MockedPeersPool);
    taker.peers.reset(new MockedPeersPool);

    maker.peer = makerPeer.get();
    taker.peer = takerPeer.get();
    maker.peers->addPeer(std::move(takerPeer));
    taker.peers->addPeer(std::move(makerPeer));

    maker.clients.reset(new MockedClientsPool(*maker.clientFactory));
    taker.clients.reset(new MockedClientsPool(*taker.clientFactory));

    auto btcCurrency = "BTC";
    auto ltcCurrency = "ETH";

    std::unique_ptr<MockedSwapClient> makerClientBtc(new MockedSwapClient(btcCurrency, "maker"));
    std::unique_ptr<MockedSwapClient> makerClientLtc(new MockedSwapClient(ltcCurrency, "maker"));
    std::unique_ptr<MockedSwapClient> takerClientBtc(new MockedSwapClient(btcCurrency, "taker"));
    std::unique_ptr<MockedSwapClient> takerClientLtc(new MockedSwapClient(ltcCurrency, "taker"));

    auto connectClients = [](MockedSwapClient* left, MockedSwapClient* right) {
        left->connectedClient = right;
        right->connectedClient = left;
    };
    connectClients(makerClientBtc.get(), takerClientBtc.get());
    connectClients(makerClientLtc.get(), takerClientLtc.get());

    auto btcRoute = MockedSwapClient::GenerateRoute("", 1, makerClientBtc->height);
    auto ltcRoute = MockedSwapClient::GenerateRoute("", 2, makerClientLtc->height);
    makerClientBtc->routes.emplace_back(btcRoute);
    takerClientBtc->routes.emplace_back(btcRoute);
    makerClientLtc->routes.emplace_back(ltcRoute);
    takerClientLtc->routes.emplace_back(ltcRoute);

    maker.clients->addClient(btcCurrency, std::move(makerClientBtc));
    maker.clients->addClient(ltcCurrency, std::move(makerClientLtc));
    taker.clients->addClient(btcCurrency, std::move(takerClientBtc));
    taker.clients->addClient(ltcCurrency, std::move(takerClientLtc));

    maker.manager.reset(
        new SwapManager(*maker.peers, *maker.clients, *maker.repository, *maker.orderbook));
    taker.manager.reset(
        new SwapManager(*taker.peers, *taker.clients, *taker.repository, *taker.orderbook));
}

//==============================================================================

void SwapTests::TearDown()
{
    auto clear = [](SwapObjects& obj) {
        SwapObjects tmp;
        std::swap(obj, tmp);
    };
    clear(maker);
    clear(taker);
}

//==============================================================================

TEST_F(SwapTests, executeSwap)
{
    // XSN_LTC
    // Buy:
    // price 0.0001 LTC = 10000 LTC
    // amount 0.001 XSN = 100000 XSN
    // total 0.00000010 LTC = 10 LTC
    // Sell:
    // price 0.0001 LTC
    // amount 0.001 XSN
    // total 0.00000010 LTC
    QObject::connect(taker.manager.get(), &swaps::SwapManager::swapFailed, [](auto deal) {
        LogDebug() << "Taker manager, swap failed:" << static_cast<int>(deal.failureReason.get());
    });
    QObject::connect(maker.manager.get(), &swaps::SwapManager::swapFailed, [](auto deal) {
        LogDebug() << "Maker manager, swap failed:" << static_cast<int>(deal.failureReason.get());
    });

    auto runSwaps = [this](size_t count = 1) {
        std::vector<Promise<void>> promises;

        for (size_t i = 0; i < count; ++i) {
            PeerOrder makerOrder;
            OwnOrder takerOrder;

            std::tie(takerOrder, makerOrder) = createDefaultTestOrder();

            {
                orderbook::OwnOrder makerOwnOrder = ConvertToOrderbookOrder(makerOrder);
                maker.orderbook->_orders[makerOrder.pairId].emplace(
                    makerOwnOrder.id, makerOwnOrder);
            }
            {

                orderbook::OwnOrder takerOwnOrder = ConvertToOrderbookOrder(takerOrder);
                taker.orderbook->_orders[takerOrder.pairId].emplace(
                    takerOwnOrder.id, takerOwnOrder);
            }

            promises.emplace_back(taker.manager->executeSwap(makerOrder, takerOrder).then([] {}));
        }

        return promises;
    };

    QSignalSpy makerSwapPaid(maker.manager.get(), &swaps::SwapManager::swapPaid);
    QSignalSpy takerSwapPaid(taker.manager.get(), &swaps::SwapManager::swapPaid);

    int count = 2;
    auto prom = QtPromise::all(runSwaps(count)).wait();

    ASSERT_FALSE(prom.isRejected());
    ASSERT_FALSE(taker.repository->_deals.empty());
    ASSERT_FALSE(maker.repository->_deals.empty());

    QObject dummy;
    QSignalSpy dummySpy(&dummy, &QObject::destroyed);
    dummySpy.wait(1000);

    // it means that both parties have finished the swap
    ASSERT_EQ(maker.repository->_deals.begin()->second.phase, SwapPhase::SwapCompleted);
    ASSERT_EQ(taker.repository->_deals.begin()->second.phase, SwapPhase::SwapCompleted);
    ASSERT_EQ(makerSwapPaid.count(), count);
    ASSERT_EQ(takerSwapPaid.count(), count);
}

TEST_F(SwapTests, executeSwapImmediateFailure)
{
    PeerOrder makerOrder;
    OwnOrder takerOrder;

    std::tie(takerOrder, makerOrder) = createDefaultTestOrder();
    taker.clients->_swapClients
        .clear(); // this will trigger error because there is no swap client for currency

    SwapFailureReason reason{ SwapFailureReason::UnknownError };
    auto prom = taker.manager->executeSwap(makerOrder, takerOrder)
                    .tapFail([&reason](const SwapFailure& ex) { reason = ex.failureReason; })
                    .wait();

    QSignalSpy swapFailure(maker.peers.get(), &AbstractSwapPeerPool::swapFailReceived);
    ASSERT_TRUE(swapFailure.wait(1000));

    ASSERT_EQ(reason, SwapFailureReason::SwapClientNotSetup);
}

TEST_F(SwapTests, lndConnextSwap)
{
    auto lndExchangeA = CreateAndConnectLndClient("localhost:20000", "/tmp/lnd-a/btc/tls.cert");

    ASSERT_FALSE(std::get<1>(lndExchangeA)->verifyConnection().timeout(5000).wait().isRejected());

    auto lndExchangeB = CreateAndConnectLndClient("localhost:20002", "/tmp/lnd-b/btc/tls.cert");

    ASSERT_FALSE(std::get<1>(lndExchangeB)->verifyConnection().timeout(5000).wait().isRejected());

    maker.clients->_swapClients.clear();
    taker.clients->_swapClients.clear();

    maker.clients->addClient("BTC", std::move(std::get<1>(lndExchangeA)));
    taker.clients->addClient("BTC", std::move(std::get<1>(lndExchangeB)));

    QNetworkAccessManager accessManager;

    RequestHandlerImpl::Domains domainsA, domainsB;
    domainsA.normal = domainsA.tor = "127.0.0.1:8001";
    domainsB.normal = domainsB.tor = "127.0.0.1:8002";

    ConnextHttpClient httpClient1(
        std::make_unique<RequestHandlerImpl>(&accessManager, nullptr, domainsA));
    ConnextHttpClient httpClient2(
        std::make_unique<RequestHandlerImpl>(&accessManager, nullptr, domainsB));

    maker.clients->_connextClient = std::make_unique<swaps::ConnextSwapClient>(&httpClient1, "ETH");
    ASSERT_FALSE(
        maker.clients->connextClient()->verifyConnection().timeout(5000).wait().isRejected());

    taker.clients->_connextClient = std::make_unique<swaps::ConnextSwapClient>(&httpClient2, "ETH");
    ASSERT_FALSE(
        taker.clients->connextClient()->verifyConnection().timeout(5000).wait().isRejected());

    auto token = "0x0000000000000000000000000000000000000000";

    maker.clients->initConnextTokens(token);
    taker.clients->initConnextTokens(token);

    maker.clients->startConnextResolver("192.168.2.1", 1235);
    taker.clients->startConnextResolver("192.168.0.1", 1236);

    QObject::connect(maker.clients->_connextResolver,
        &swaps::ConnextHttpResolveService::receivedResolvedTransfer,
        maker.clients->_connextClient.get(), &AbstractSwapConnextClient::onTransferResolved);

    QObject::connect(taker.clients->_connextResolver,
        &swaps::ConnextHttpResolveService::receivedResolvedTransfer,
        taker.clients->_connextClient.get(), &AbstractSwapConnextClient::onTransferResolved);

    PeerOrder makerOrder;
    OwnOrder takerOrder;

    // if BTC_ETH and isBuy = true then user sends BTC and receives ETH
    std::tie(takerOrder, makerOrder) = CreateOrderPair(
        "BTC_ETH", false, 1 * std::pow(10, 6), std::make_pair(100, 100), maker.peer->nodePubKey());

    {
        orderbook::OwnOrder makerOwnOrder = ConvertToOrderbookOrder(makerOrder);
        maker.orderbook->_orders[makerOrder.pairId].emplace(makerOwnOrder.id, makerOwnOrder);
    }
    {

        orderbook::OwnOrder takerOwnOrder = ConvertToOrderbookOrder(takerOrder);
        taker.orderbook->_orders[takerOrder.pairId].emplace(takerOwnOrder.id, takerOwnOrder);
    }

    QObject::connect(taker.manager.get(), &swaps::SwapManager::swapFailed, [](auto deal) {
        LogDebug() << "Taker manager, swap failed:" << static_cast<int>(deal.failureReason.get());
    });
    QObject::connect(maker.manager.get(), &swaps::SwapManager::swapFailed, [](auto deal) {
        LogDebug() << "Maker manager, swap failed:" << static_cast<int>(deal.failureReason.get());
    });
    auto prom = taker.manager->executeSwap(makerOrder, takerOrder).wait();
    ASSERT_FALSE(prom.isRejected());
}

TEST_F(OrderbookClientTests, OrderPostingTest)
{
    startAndGetPairId(*makerOrderbook, pairId);
    PeerOrder makerOrder;
    OwnOrder takerOrder;

    const std::string makerPubKey{ "maker_pub_key" };

    std::tie(takerOrder, makerOrder) = SwapTests::CreateOrderPair(
        pairId, true, 1 * std::pow(10, 6), std::make_pair(10000, 10000), makerPubKey);

    orderbook::OwnOrder makerOwnOrder = ConvertToOrderbookOrder(makerOrder);

    makerOrderbook->placeOrder(makerOwnOrder)
        .then([makerOwnOrder, this](orderbook::PlaceOrderOutcome outcome) {
            auto ownOrder = boost::get<orderbook::OwnOrder>(&outcome);
            ASSERT_TRUE(ownOrder);
            ASSERT_EQ(makerOwnOrder.pairId, ownOrder->pairId);
            ASSERT_FALSE(ownOrder->id.empty());
            ASSERT_TRUE(ownOrder->openPortions.empty());
            ASSERT_EQ(makerOwnOrder.quantity, ownOrder->quantity);
            ASSERT_EQ(makerOwnOrder.price, ownOrder->price);

            ASSERT_TRUE(
                makerOrderbook->tryGetOwnOrder(ownOrder->pairId, ownOrder->id).is_initialized());
            ASSERT_FALSE(makerOrderbook->tryGetOrderPortion(ownOrder->pairId, ownOrder->id)
                             .is_initialized());
            ASSERT_FALSE(
                makerOrderbook->tryGetBaseOrder(ownOrder->pairId, ownOrder->id).is_initialized());
            EXPECT_NO_THROW(makerOrderbook->getOwnOrder(ownOrder->pairId, ownOrder->id));
        })
        .wait();
}

TEST_F(OrderbookClientTests, OrderPartialOrderPostingTest)
{
    startAndGetPairId(*makerOrderbook, pairId);
    PeerOrder makerOrder;
    OwnOrder takerOrder;

    const std::string makerPubKey{ "maker_pub_key" };

    std::tie(takerOrder, makerOrder) = SwapTests::CreateOrderPair(
        pairId, true, 1 * std::pow(10, 6), std::make_pair(10000, 10000), makerPubKey);
    orderbook::OwnOrder makerOwnOrder = ConvertToOrderbookOrder(makerOrder);
    std::string postedOrderId;
    makerOrderbook->placeOrder(makerOwnOrder)
        .then([&postedOrderId](orderbook::PlaceOrderOutcome outcome) {
            postedOrderId = boost::get<orderbook::OwnOrder>(&outcome)->id;
        })
        .wait();

    const int64_t diffAmount = 5000;

    ASSERT_GT(makerOwnOrder.quantity, diffAmount);

    makerOrderbook
        ->placeOrderPortion(makerOwnOrder.pairId, postedOrderId,
            makerOwnOrder.quantity - diffAmount, orderbook::OwnOrderType::Limit)
        .then([makerOwnOrder, postedOrderId, diffAmount, this](
                  orderbook::PlaceOrderOutcome outcome) {
            auto portion = boost::get<orderbook::OrderPortion>(&outcome);
            ASSERT_TRUE(portion);
            ASSERT_EQ(makerOwnOrder.quantity - diffAmount, portion->quantity);
            ASSERT_NE(portion->orderId, postedOrderId);

            auto baseOrder
                = makerOrderbook->tryGetBaseOrder(makerOwnOrder.pairId, portion->orderId);
            auto ownPartialOrder
                = makerOrderbook->tryGetOrderPortion(makerOwnOrder.pairId, portion->orderId);

            ASSERT_TRUE(ownPartialOrder.is_initialized());
            ASSERT_FALSE(makerOrderbook->tryGetOwnOrder(makerOwnOrder.pairId, portion->orderId)
                             .is_initialized());
            ASSERT_TRUE(baseOrder.is_initialized());
            EXPECT_NO_THROW(makerOrderbook->getOwnOrder(makerOwnOrder.pairId, portion->orderId));

            ASSERT_EQ(baseOrder->openPortions.back().orderId, portion->orderId);
            ASSERT_EQ(baseOrder->openPortions.back().quantity, portion->quantity);
            ASSERT_EQ(ownPartialOrder->pairId, baseOrder->pairId);
            ASSERT_EQ(ownPartialOrder->price, baseOrder->price);
            ASSERT_EQ(ownPartialOrder->isOwnOrder, baseOrder->isOwnOrder);
            ASSERT_LT(ownPartialOrder->quantity, baseOrder->quantity);
            ASSERT_NE(ownPartialOrder->id, baseOrder->id);
        })
        .wait();
}

TEST_F(OrderbookClientTests, OrderMatched)
{
    startAndGetPairId(*makerOrderbook, pairId);
    startAndGetPairId(*takerOrderbook, pairId);
    PeerOrder makerOrder;
    OwnOrder takerOrder;

    const std::string makerPubKey{ "maker_pub_key" };

    std::tie(takerOrder, makerOrder) = SwapTests::CreateOrderPair(
        pairId, true, 1 * std::pow(10, 6), std::make_pair(10000, 10000), makerPubKey);

    orderbook::OwnOrder makerOwnOrder = ConvertToOrderbookOrder(makerOrder);
    std::string postedOrderId;
    makerOrderbook->placeOrder(makerOwnOrder)
        .then([&postedOrderId](orderbook::PlaceOrderOutcome outcome) {
            postedOrderId = boost::get<orderbook::OwnOrder>(&outcome)->id;
        })
        .wait();

    ASSERT_FALSE(makerOrderbook->tryGetMatchedTrade(pairId, postedOrderId).is_initialized());
    orderbook::OwnOrder takerOwnOrder = ConvertToOrderbookOrder(takerOrder);

    takerOrderbook->placeOrder(takerOwnOrder)
        .then([this](orderbook::PlaceOrderOutcome outcome) {
            auto match = boost::get<orderbook::Trade>(&outcome);
            ASSERT_TRUE(match);
            auto bobTrade = takerOrderbook->tryGetMatchedTrade(pairId, match->id);
            ASSERT_TRUE(bobTrade.is_initialized());
        })
        .wait();

    QEventLoop loop;
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    ASSERT_TRUE(makerOrderbook->tryGetMatchedTrade(pairId, postedOrderId).is_initialized());
}

TEST_F(OrderbookClientTests, OrderbookSendMessageEvent)
{
    startAndGetPairId(*makerOrderbook, pairId);
    startAndGetPairId(*takerOrderbook, pairId);
    PeerOrder makerOrder;
    OwnOrder takerOrder;

    const std::string makerPubKey{ "maker_pub_key" };

    std::tie(takerOrder, makerOrder) = SwapTests::CreateOrderPair(
        pairId, true, 1 * std::pow(10, 6), std::make_pair(10000, 10000), makerPubKey);

    orderbook::OwnOrder makerOwnOrder = ConvertToOrderbookOrder(makerOrder);
    std::string postedOrderId;
    makerOrderbook->placeOrder(makerOwnOrder)
        .then([&postedOrderId](orderbook::PlaceOrderOutcome outcome) {
            postedOrderId = boost::get<orderbook::OwnOrder>(&outcome)->id;
        })
        .wait();

    orderbook::OwnOrder takerOwnOrder = ConvertToOrderbookOrder(takerOrder);

    std::string takerOrderId;
    takerOrderbook->placeOrder(takerOwnOrder)
        .then([&takerOrderId](orderbook::PlaceOrderOutcome outcome) {
            auto match = boost::get<orderbook::Trade>(&outcome);
            takerOrderId = match->id;
        })
        .wait();

    QEventLoop loop;
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    using io::stakenet::orderbook::protos::Event;

    size_t numberOfEvents = 0;
    const std::string msg("orderbookCmnd");
    makerOrderbook->dispatcher().addEventHandler(Event::ServerEvent::ValueCase::kNewOrderMessage,
        [&numberOfEvents, msg](Event::ServerEvent event) {
            ++numberOfEvents;
            ASSERT_EQ(msg, event.newordermessage().message());
        });

    ASSERT_TRUE(makerOrderbook->tryGetMatchedTrade(pairId, postedOrderId).is_initialized());

    takerOrderbook
        ->sendOrderMessage(takerOrderId, std::vector<unsigned char>(msg.begin(), msg.end()))
        .timeout(5000)
        .wait();

    ASSERT_EQ(numberOfEvents, 1);
}

TEST_F(OrderbookClientTests, OwnOrderSwapCompleted)
{
    startAndGetPairId(*makerOrderbook, pairId);
    startAndGetPairId(*takerOrderbook, pairId);
    PeerOrder makerOrder;
    OwnOrder takerOrder;

    const std::string makerPubKey{ "maker_pub_key" };

    std::tie(takerOrder, makerOrder) = SwapTests::CreateOrderPair(
        pairId, true, 1 * std::pow(10, 6), std::make_pair(10000, 10000), makerPubKey);

    orderbook::OwnOrder makerOwnOrder = ConvertToOrderbookOrder(makerOrder);
    std::string makerOrderId;
    makerOrderbook->placeOrder(makerOwnOrder)
        .then([&makerOrderId](orderbook::PlaceOrderOutcome outcome) {
            makerOrderId = boost::get<orderbook::OwnOrder>(&outcome)->id;
        })
        .wait();

    orderbook::OwnOrder takerOwnOrder = ConvertToOrderbookOrder(takerOrder);
    std::string takerOrderId;
    takerOrderbook->placeOrder(takerOwnOrder)
        .then([&takerOrderId](orderbook::PlaceOrderOutcome outcome) {
            takerOrderId = boost::get<orderbook::Trade>(outcome).id;
        })
        .wait();

    QEventLoop loop;
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    ASSERT_TRUE(makerOrderbook->tryGetMatchedTrade(pairId, makerOrderId).is_initialized());
    ASSERT_TRUE(takerOrderbook->tryGetMatchedTrade(pairId, takerOrderId).is_initialized());

    SwapSuccess takerSuccess;
    takerSuccess.pairId = pairId;
    takerSuccess.orderId = takerOrderId;

    SwapSuccess makerSuccess;
    makerSuccess.pairId = pairId;
    makerSuccess.orderId = makerOrderId;

    takerOrderbook->onOwnOrderSwapSuccess(takerSuccess);
    makerOrderbook->onOwnOrderSwapSuccess(makerSuccess);

    ASSERT_FALSE(makerOrderbook->tryGetMatchedTrade(pairId, makerOrderId).is_initialized());
    ASSERT_FALSE(takerOrderbook->tryGetMatchedTrade(pairId, takerOrderId).is_initialized());
    auto ownOrder = makerOrderbook->tryGetOwnOrder(pairId, makerOrderId);
    ASSERT_TRUE(ownOrder.is_initialized());

    makerOrderbook->onOwnOrderCompleted(*ownOrder);

    ASSERT_FALSE(makerOrderbook->tryGetOwnOrder(pairId, makerOrderId));
}

TEST_F(OrderbookClientTests, PartialOrderMatching)
{
    startAndGetPairId(*makerOrderbook, pairId);
    startAndGetPairId(*takerOrderbook, pairId);
    PeerOrder makerOrder;
    OwnOrder takerOrder;

    const std::string makerPubKey{ "maker_pub_key" };

    std::tie(takerOrder, makerOrder) = SwapTests::CreateOrderPair(
        pairId, true, 1 * std::pow(10, 6), std::make_pair(10000, 10000), makerPubKey);

    orderbook::OwnOrder makerOwnOrder = ConvertToOrderbookOrder(makerOrder);
    std::string makerOrderId;
    makerOrderbook->placeOrder(makerOwnOrder)
        .then([&makerOrderId](orderbook::PlaceOrderOutcome outcome) {
            makerOrderId = boost::get<orderbook::OwnOrder>(&outcome)->id;
        })
        .wait();

    orderbook::OwnOrder takerOwnOrder = ConvertToOrderbookOrder(takerOrder);
    const int64_t diffAmount = 5000;
    takerOwnOrder.quantity = diffAmount;
    std::string takerOrderId;
    takerOrderbook->placeOrder(takerOwnOrder)
        .then([&takerOrderId](orderbook::PlaceOrderOutcome outcome) {
            takerOrderId = boost::get<orderbook::Trade>(outcome).id;
        })
        .wait();

    // after this one we have to post maker order for amount - diffAmount and takerOrder for
    // diffAmount

    QEventLoop loop;
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    SwapSuccess takerSuccess;
    takerSuccess.pairId = pairId;
    takerSuccess.orderId = takerOrderId;
    takerSuccess.amountReceived = takerOwnOrder.quantity;

    SwapSuccess makerSuccess;
    makerSuccess.pairId = pairId;
    makerSuccess.orderId = makerOrderId;
    makerSuccess.amountSent = takerOwnOrder.quantity;

    takerOrderbook->onOwnOrderSwapSuccess(takerSuccess);
    makerOrderbook->onOwnOrderSwapSuccess(makerSuccess);

    takerOwnOrder.quantity = makerOwnOrder.quantity - takerOwnOrder.quantity;

    QSignalSpy makerOrderChanged(
        makerOrderbook.get(), &orderbook::OrderbookClient::ownOrderChanged);
    std::string makerPortionId;
    ASSERT_FALSE(makerOrderbook
                     ->placeOrderPortion(pairId, makerOrderId, takerOwnOrder.quantity,
                         orderbook::OwnOrderType::Limit)
                     .then([&makerPortionId](orderbook::PlaceOrderOutcome outcome) {
                         makerPortionId = boost::get<orderbook::OrderPortion>(outcome).orderId;
                     })
                     .wait()
                     .isRejected());

    ASSERT_EQ(makerOrderChanged.count(), 1);

    ASSERT_FALSE(takerOrderbook->placeOrder(takerOwnOrder)
                     .then([&takerOrderId](orderbook::PlaceOrderOutcome outcome) {
                         takerOrderId = boost::get<orderbook::Trade>(outcome).id;
                     })
                     .wait()
                     .isRejected());

    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    loop.exec();

    ASSERT_TRUE(makerOrderbook->tryGetMatchedTrade(pairId, makerPortionId).is_initialized());
    ASSERT_TRUE(takerOrderbook->tryGetMatchedTrade(pairId, takerOrderId).is_initialized());

    ASSERT_TRUE(makerOrderbook->tryGetOrderPortion(pairId, makerPortionId).is_initialized());
    auto baseOrder = makerOrderbook->tryGetBaseOrder(pairId, makerPortionId);
    ASSERT_TRUE(baseOrder.is_initialized());
    ASSERT_EQ(baseOrder->openPortions.size(), 1);
    ASSERT_EQ(baseOrder->closedPortions.size(), 1);
    ASSERT_EQ(baseOrder->closedPortions.front().quantity, takerOwnOrder.quantity);
    ASSERT_EQ(baseOrder->openPortions.back().orderId, makerPortionId);

    makerSuccess.orderId = makerPortionId;
    makerOrderbook->onOwnOrderSwapSuccess(makerSuccess);

    ASSERT_FALSE(makerOrderbook->tryGetMatchedTrade(pairId, makerPortionId).is_initialized());

    QSignalSpy makerOrderCompleted(
        makerOrderbook.get(), &orderbook::OrderbookClient::ownOrderCompleted);
    makerOrderbook->onOwnOrderCompleted(baseOrder.get());
    ASSERT_FALSE(makerOrderbook->tryGetOwnOrder(pairId, baseOrder->id).is_initialized());
    ASSERT_EQ(makerOrderCompleted.count(), 1);
}
