#include "TradingBotExchangeGateway.hpp"
#include <Models/DexService.hpp>
#include <Models/OrderBookListModel.hpp>
#include <Models/WalletDexStateModel.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <SwapService.hpp>
#include <Utils/Utils.hpp>

//==============================================================================

TradingBotExchangeGateway::TradingBotExchangeGateway(const DexService& dexService, QObject* parent)
    : tradingbot::gateway::ExchangeGateway{ parent }
    , _dexService(dexService)
{

    using orderbook::OrderbookClient;
    auto orderbookClient = _dexService.orderbook();

    connect(orderbookClient, &OrderbookClient::ownOrderCompleted, this,
        [this](std::string orderId) { this->orderCompleted(orderId); });
    connect(orderbookClient, &OrderbookClient::ownOrderChanged, this,
        [this](orderbook::OwnOrder order) { this->orderChanged(order); });
}

//==============================================================================

double TradingBotExchangeGateway::getLastPrice(QString pairId) const
{
    auto& stateModel = _dexService.stateModel();
    if (stateModel.getPairId() != pairId) {
        return 0.0;
    }

    auto bestBuyPrice = stateModel.buyOrdersListModel()->bestPrice();
    auto bestSellPrice = stateModel.sellOrdersListModel()->bestPrice();

    // TODO: check division by COIN, maybe that's wrong for ETH assets

    return (bestBuyPrice + bestSellPrice) / 2.0 / COIN;
}

//==============================================================================

Promise<tradingbot::gateway::PlaceOrderOutcome> TradingBotExchangeGateway::placeOrder(
    orderbook::LimitOrder order)
{
    using tradingbot::gateway::ExecutedOwnOrder;
    using tradingbot::gateway::PlaceOrderOutcome;
    return _dexService.swapService().placeOrder(order).then(
        [isBuy = order.isBuy](swaps::PlaceOrderResult outcome) {
            if (auto ownOrder = boost::get<orderbook::OwnOrder>(&outcome)) {
                return PlaceOrderOutcome{ *ownOrder };
            } else if (auto success = boost::get<swaps::SwapSuccess>(&outcome)) {
                ExecutedOwnOrder executedOwnOrder;
                executedOwnOrder.pairId = QString::fromStdString(success->pairId);
                executedOwnOrder.localId = QString::fromStdString(success->localId);
                executedOwnOrder.orderId = QString::fromStdString(success->orderId);
                executedOwnOrder.price = success->price;
                executedOwnOrder.quantity = isBuy ? success->amountReceived : success->amountSent;
                return PlaceOrderOutcome{ executedOwnOrder };
            }

            Q_ASSERT_X(false, __FUNCTION__, "Variant type is not expected");
            return PlaceOrderOutcome{ ExecutedOwnOrder{} };
        });
}

//==============================================================================

Promise<void> TradingBotExchangeGateway::cancelOrder(std::string pairId, std::string localId) const
{
    return _dexService.swapService().cancelOrder(pairId, localId);
}

//==============================================================================

boost::optional<orderbook::OwnOrder> TradingBotExchangeGateway::tryGetOwnOrder(
    std::string pairId, std::string orderId) const
{
    auto orderbookClient = _dexService.orderbook();
    Q_ASSERT_X(orderbookClient->thread() == QThread::currentThread(), __FUNCTION__,
        "Calling sync method from different thread");

    return orderbookClient->tryGetOwnOrder(pairId, orderId);
}

//==============================================================================
