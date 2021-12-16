#include <QDateTime>
#include <QUuid>
#include <TradingBot/ExchangeGateway.hpp>
#include <TradingBot/MovingGridStrategy.hpp>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

using ::testing::InSequence;
using ::testing::NiceMock;
using ::testing::Return;

//==============================================================================

/*!
 * \brief WaitFor is needed to wait for promises delivery as it happens async using Qt event loop
 * \param timeoutMs
 */
static void WaitFor(uint64_t timeoutMs)
{
    QEventLoop loop;
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
}

//==============================================================================

class MockExchangeGateway : public tradingbot::gateway::ExchangeGateway {
public:
    MOCK_METHOD(double, getLastPrice, (QString pairId), (const, override));
    MOCK_METHOD(Promise<tradingbot::gateway::PlaceOrderOutcome>, placeOrder,
        (orderbook::LimitOrder order), (override));
    MOCK_METHOD(boost::optional<orderbook::OwnOrder>, tryGetOwnOrder,
        (std::string pairId, std::string orderId), (const, override));
    MOCK_METHOD(Promise<void>, cancelOrder, (std::string pairId, std::string localId), (const, override));
};

//==============================================================================

struct TradingBotTestSuite : public ::testing::Test {
    TradingBotTestSuite() {}

    std::unique_ptr<tradingbot::MovingGridStrategy> strategy;
    MockExchangeGateway* gateway{ nullptr };
    tradingbot::MovingGridStrategy::Config cfg;

    // Test interface
protected:
    void SetUp() override
    {
        cfg.pairId = "XSN_BTC";
        cfg.levels = 5;
        cfg.interval = 0.1;

        auto g = std::make_unique<NiceMock<MockExchangeGateway>>();
        gateway = g.get();
        strategy = std::make_unique<tradingbot::MovingGridStrategy>(cfg, std::move(g));
    }
    void TearDown() override {}
};

//==============================================================================

orderbook::OwnOrder OwnOrderFixture(orderbook::LimitOrder limitOrder)
{
    auto uuid = QUuid::createUuid().toString();
    auto orderbookUUid = QUuid::createUuid().toString();

    orderbook::OwnOrder ownOrder(
        orderbook::OwnLimitOrder(limitOrder, orderbook::Local(uuid.toStdString(), 0)),
        orderbook::Stamp(orderbook::OrderIdentifier(orderbookUUid.toStdString()),
            QDateTime::currentSecsSinceEpoch(), limitOrder.quantity),
        true);
    return ownOrder;
}

//==============================================================================

tradingbot::gateway::ExecutedOwnOrder ExecuteOwnOrderFixture(orderbook::OwnOrder ownOrder)
{
    tradingbot::gateway::ExecutedOwnOrder executedOrder;
    executedOrder.localId = QString::fromStdString(ownOrder.localId);
    executedOrder.orderId = QString::fromStdString(ownOrder.id);
    executedOrder.pairId = QString::fromStdString(ownOrder.pairId);
    executedOrder.price = ownOrder.price;
    executedOrder.quantity = ownOrder.quantity;
    return executedOrder;
}

//==============================================================================

tradingbot::gateway::ExecutedOwnOrder ExecuteOwnOrderFixture(orderbook::LimitOrder limitOrder)
{
    auto uuid = QUuid::createUuid().toString();
    auto orderbookUUid = QUuid::createUuid().toString();

    tradingbot::gateway::ExecutedOwnOrder executedOrder;
    executedOrder.localId = uuid;
    executedOrder.orderId = orderbookUUid;
    executedOrder.pairId = QString::fromStdString(limitOrder.pairId);
    executedOrder.price = limitOrder.price;
    executedOrder.quantity = limitOrder.quantity;

    return executedOrder;
}

//==============================================================================

TEST_F(TradingBotTestSuite, TestMovingGridInitialize)
{
    static const auto basePrice = 0.001;
    ON_CALL(*gateway, getLastPrice).WillByDefault([this](QString pairId) -> double {
        EXPECT_EQ(cfg.pairId, pairId);
        return basePrice;
    });
    {
        InSequence s;
        for (uint32_t level = 1; level <= cfg.levels; ++level) {
            // buy orders
            EXPECT_CALL(*gateway, placeOrder)
                .WillOnce([this, level](orderbook::LimitOrder limitOrder) {
                    EXPECT_EQ(cfg.pairId.toStdString(), limitOrder.pairId);
                    EXPECT_EQ(true, limitOrder.isBuy);
                    auto price = qRound(
                        (basePrice * std::pow(1.0 / (1 + cfg.interval), level)) * std::pow(10, 8));
                    EXPECT_EQ(price, limitOrder.price);

                    return Promise<tradingbot::gateway::PlaceOrderOutcome>::resolve(
                        OwnOrderFixture(limitOrder));
                })
                .RetiresOnSaturation();
        }

        for (uint32_t level = cfg.levels; level > 0; --level) {
            // sell orders
            EXPECT_CALL(*gateway, placeOrder)
                .WillOnce([this, level](orderbook::LimitOrder limitOrder) {
                    EXPECT_EQ(cfg.pairId.toStdString(), limitOrder.pairId);
                    EXPECT_EQ(false, limitOrder.isBuy);
                    auto price
                        = qRound((basePrice * std::pow(1 + cfg.interval, level)) * std::pow(10, 8));
                    EXPECT_EQ(price, limitOrder.price);
                    return Promise<tradingbot::gateway::PlaceOrderOutcome>::resolve(
                        OwnOrderFixture(limitOrder));
                })
                .RetiresOnSaturation();
        }
    }
    strategy->start();
}

//==============================================================================

TEST_F(TradingBotTestSuite, TestMovingGridStrategyMakerStep)
{
    static auto basePrice = 0.001;
    ON_CALL(*gateway, getLastPrice).WillByDefault([this](QString pairId) -> double {
        EXPECT_EQ(cfg.pairId, pairId);
        return basePrice;
    });
    std::vector<orderbook::OwnOrder> postedSellOrders;
    EXPECT_CALL(*gateway, placeOrder)
        .Times(cfg.levels * 2)
        .WillRepeatedly([&](orderbook::LimitOrder limitOrder)
                            -> Promise<tradingbot::gateway::PlaceOrderOutcome> {
            auto ownOrder = OwnOrderFixture(limitOrder);
            if (!limitOrder.isBuy) {
                postedSellOrders.emplace_back(ownOrder);
            }
            return Promise<tradingbot::gateway::PlaceOrderOutcome>::resolve(ownOrder);
        })
        .RetiresOnSaturation();

    InSequence s;
    strategy->start();

    // wait for promises delivering
    WaitFor(50);

    for (size_t i = 0; i < postedSellOrders.size(); ++i) {
        const auto& matchedOrder = postedSellOrders[postedSellOrders.size() - i - 1];
        auto localId = matchedOrder.localId;
        basePrice
            = matchedOrder.price / std::pow(10, 8); // change basePrice to emulate new market price
        auto expectedPrice
            = qRound((basePrice * std::pow(1.0 / (1 + cfg.interval), 1)) * std::pow(10, 8));

        EXPECT_CALL(*gateway, placeOrder)
            .WillOnce([expectedPrice](orderbook::LimitOrder limitOrder) {
                EXPECT_TRUE(limitOrder.isBuy);
                EXPECT_EQ(limitOrder.price, expectedPrice);
                return Promise<tradingbot::gateway::PlaceOrderOutcome>::resolve(
                    OwnOrderFixture(limitOrder));
            });
        // trigger filling of order
        gateway->orderCompleted(localId);
    }

    // wait for promises delivering
    WaitFor(50);
}

//==============================================================================

TEST_F(TradingBotTestSuite, TestMovingGridStrategyTakerStep)
{
    static auto basePrice = 0.001;
    ON_CALL(*gateway, getLastPrice).WillByDefault([this](QString pairId) -> double {
        EXPECT_EQ(cfg.pairId, pairId);
        return basePrice;
    });
    std::map<std::string, orderbook::OwnOrder> ownOrders;
    ON_CALL(*gateway, tryGetOwnOrder)
        .WillByDefault([this, &ownOrders](std::string pairId,
                           std::string orderId) -> boost::optional<orderbook::OwnOrder> {
            EXPECT_EQ(cfg.pairId.toStdString(), pairId);
            return ownOrders.count(orderId) > 0 ? boost::make_optional(ownOrders.at(orderId))
                                                : boost::none;
        });
    std::vector<tradingbot::gateway::ExecutedOwnOrder> postedBuyOrders;
    EXPECT_CALL(*gateway, placeOrder)
        .Times(cfg.levels * 2)
        .WillRepeatedly([&](orderbook::LimitOrder limitOrder)
                            -> Promise<tradingbot::gateway::PlaceOrderOutcome> {
            auto ownOrder = OwnOrderFixture(limitOrder);
            ownOrders.emplace(ownOrder.localId, ownOrder);
            if (limitOrder.isBuy) {
                auto executedOwnOrder = ExecuteOwnOrderFixture(ownOrder);
                postedBuyOrders.emplace_back(executedOwnOrder);
                return Promise<tradingbot::gateway::PlaceOrderOutcome>::resolve(executedOwnOrder);
            } else {
                return Promise<tradingbot::gateway::PlaceOrderOutcome>::resolve(ownOrder);
            }
        })
        .RetiresOnSaturation();

    InSequence s;
    strategy->start();

    // wait for promises delivering
    WaitFor(50);

    for (size_t i = 0; i < postedBuyOrders.size(); ++i) {
        auto executedOrder = postedBuyOrders[i];
        basePrice
            = executedOrder.price / std::pow(10, 8); // change basePrice to emulate new market price
        auto expectedPrice = qRound((basePrice * std::pow(1 + cfg.interval, 1)) * std::pow(10, 8));
        EXPECT_CALL(*gateway, placeOrder)
            .WillOnce([expectedPrice](orderbook::LimitOrder limitOrder) {
                EXPECT_TRUE(!limitOrder.isBuy);
                EXPECT_EQ(limitOrder.price, expectedPrice);
                return Promise<tradingbot::gateway::PlaceOrderOutcome>::resolve(
                    OwnOrderFixture(limitOrder));
            })
            .RetiresOnSaturation();
        // trigger filling of order
        gateway->orderCompleted(executedOrder.localId.toStdString());
    }

    // wait for promises delivering
    WaitFor(50);
}

//==============================================================================
