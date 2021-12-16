#include "OrderBookData.hpp"
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/TradingOrdersModel.hpp>
#include <string>

#if 0

using namespace orderbook;

//==============================================================================

static OrderEntry ParseOrder(io::stakenet::orderbook::protos::Order order)
{
    OrderEntry orderEntry;
    orderEntry.id = QString::fromStdString(order.details().orderid());
    orderEntry.pairId = QString::fromStdString(order.tradingpair());
    orderEntry.side = order.side() == 0 ? Enums::OrderSide::Buy : Enums::OrderSide::Sell;
    orderEntry.amount = ConvertBigInt(order.details().funds());
    orderEntry.type = order.type() == 0 ?  Enums::OrderType::Limit : Enums::OrderType::Market;
    if(orderEntry.type == Enums::OrderType::Limit)
    {
        orderEntry.price = ConvertBigInt(order.details().price());
    }
    return orderEntry;
}

//==============================================================================

OrderBookData::OrderBookData(QPointer<orderbook::OrderbookClient> orderbookClient, QObject *parent) :
    QObject(parent),
    _orderbookClient(orderbookClient)
{
    connect(_orderbookClient->ordersModel(), &TradingOrdersModel::ordersChanged, this, &OrderBookData::ordersChanged);
    connect(_orderbookClient, &OrderbookClient::ownOrderPlaced, [this](io::stakenet::orderbook::protos::Order order) {
        ownOrderPlaced(ParseOrder(order));
    });
    connect(_orderbookClient, &OrderbookClient::ownOrderMatched, this, &OrderBookData::ownOrderMatched);
    connect(_orderbookClient, &OrderbookClient::ownOrderCanceled, this, &OrderBookData::ownOrderCanceled);
    connect(_orderbookClient, &OrderbookClient::ownOrdersChanged, this, &OrderBookData::ownOrdersChanged);

}

//==============================================================================

OrderBookData::~OrderBookData()
{
}

//==============================================================================

Promise<std::vector<OrderEntry>> OrderBookData::ordersByPairId(QString pairId)
{
    return Promise<std::vector<OrderEntry>>([=](const auto &resolve, const auto &) {
        QMetaObject::invokeMethod(this, [=] {
            std::vector<OrderEntry> result;

            auto traverser = [&](bool isBuy, auto details) {
                OrderEntry order;
                order.id = QString::fromStdString(details.orderid());
                order.pairId = pairId;
                if(details.has_funds())
                {
                    order.amount = ConvertBigInt(details.funds());
                }
                if(details.has_price())
                {
                    order.type = Enums::OrderType::Limit;
                    order.price = ConvertBigInt(details.price());
                }
                else
                {
                    order.type = Enums::OrderType::Market;
                }
                order.side = isBuy ? Enums::OrderSide::Buy : Enums::OrderSide::Sell;
                result.push_back(order);
            };

            _orderbookClient->ordersModel()->askOrders(pairId.toStdString(), std::bind(traverser, false, std::placeholders::_1));
            _orderbookClient->ordersModel()->bidOrders(pairId.toStdString(), std::bind(traverser, true, std::placeholders::_1));

            resolve(result);
        });
    });
}

//==============================================================================

Promise<std::vector<OrderEntry> > OrderBookData::ownOrders()
{
    return Promise<std::vector<OrderEntry>>([=](const auto &resolve, const auto &) {
        QMetaObject::invokeMethod(this, [=] {
            std::vector<OrderEntry> result;

            auto traverser = [&](auto order) {
                result.push_back(ParseOrder(order));
            };

            _orderbookClient->ownOrders(std::bind(traverser, std::placeholders::_1));

            resolve(result);
        });
    });
}

//==============================================================================

OrderEntry::OrderEntry(QString id, QString pairId, Balance price, Balance amount, Enums::OrderType type, Enums::OrderSide side) :
    id(id),
    pairId(pairId),
    price(price),
    amount(amount),
    type(type),
    side(side)
{
}

//==============================================================================

#endif
