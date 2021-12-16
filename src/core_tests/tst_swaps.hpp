#ifndef TST_SWAPS_HPP
#define TST_SWAPS_HPP

#include <Swaps/AbstractSwapClient.hpp>
#include <Swaps/AbstractSwapClientPool.hpp>
#include <Swaps/AbstractSwapPeer.hpp>
#include <Swaps/AbstractSwapPeerPool.hpp>
#include <Swaps/SwapManager.hpp>
#include <Tools/Common.hpp>
#include <gtest/gtest.h>

#include <QDateTime>

using namespace swaps;
using namespace testing;

struct MockedClientsPool;
struct MockedRepository;
struct MockedPeersPool;
struct MockedPeer;
struct MockedOrderBook;
struct MockedSwapClientFactory;

namespace orderbook {
class OrderbookClient;
}

struct SwapTests : public ::testing::Test {
    SwapTests();
    struct SwapObjects {
        std::unique_ptr<SwapManager> manager;
        std::unique_ptr<MockedPeersPool> peers;
        std::unique_ptr<MockedRepository> repository;
        std::unique_ptr<MockedClientsPool> clients;
        std::unique_ptr<MockedOrderBook> orderbook;
        std::unique_ptr<MockedSwapClientFactory> clientFactory;
        MockedPeer* peer{ nullptr };
    } maker, taker;

    static std::tuple<swaps::OwnOrder, swaps::PeerOrder> CreateOrderPair(std::string pairId,
        bool isBuy, int64_t price, std::pair<int64_t, int64_t> initialQuantity,
        std::string peerPubKey);

    std::tuple<swaps::OwnOrder, swaps::PeerOrder> createDefaultTestOrder();

    // Test interface
protected:
    void SetUp() override;
    void TearDown() override;
};

struct OrderbookClientTests : public ::testing::Test {
    std::unique_ptr<orderbook::OrderbookClient> makerOrderbook;
    std::unique_ptr<orderbook::OrderbookClient> takerOrderbook;
    std::string pairId;

    OrderbookClientTests();
    void startAndGetPairId(orderbook::OrderbookClient& orderbook, std::string& pairId);
    // Test interface
protected:
    void SetUp() override;
    void TearDown() override;
};

#endif // TST_SWAPS_HPP
