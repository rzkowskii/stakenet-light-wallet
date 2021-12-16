#include "OwnOrdersDataSource.hpp"
#include <Orderbook/OrderbookClient.hpp>

using namespace orderbook;

//==============================================================================

static OwnOrderEntry ParseOrder(orderbook::OwnOrder order)
{
    OwnOrderEntry orderEntry(OrderSummary(order.price, order.quantity));
    orderEntry.id = QString::fromStdString(order.localId);
    orderEntry.pairId = QString::fromStdString(order.pairId);
    orderEntry.side = order.isBuy ? Enums::OrderSide::Buy : Enums::OrderSide::Sell;
    auto transformPortions = [](const auto& from, auto& to) {
        std::transform(
            std::begin(from), std::end(from), std::back_inserter(to), [](const auto& portion) {
                return std::make_tuple(QString::fromStdString(portion.orderId), portion.quantity);
            });
    };
    transformPortions(order.openPortions, orderEntry.openPortions);
    transformPortions(order.closedPortions, orderEntry.closedPortions);
    auto totalAmount = [](const auto& from) {
        return std::accumulate(std::begin(from), std::end(from), Balance{ 0 },
            [](auto accum, const auto& portion) { return accum + portion.quantity; });
    };

    orderEntry.openAmount = order.quantity - totalAmount(order.closedPortions);
    orderEntry.completedAmount = totalAmount(order.closedPortions);
    return orderEntry;
}

//==============================================================================

OwnOrdersDataSource::OwnOrdersDataSource(
    QPointer<orderbook::OrderbookClient> orderbook, QString pairId, QObject* parent)
    : QObject(parent)
    , _orderbook(orderbook)
    , _pairId(pairId.toStdString())
{
    connect(_orderbook, &OrderbookClient::ownOrderPlaced, this,
        [this](auto order) { this->onOwnOrderPlaced(ParseOrder(order)); });
    connect(_orderbook, &OrderbookClient::ownOrderChanged, this,
        [this](auto order) { this->onOwnOrderChanged(ParseOrder(order)); });
    connect(_orderbook, &OrderbookClient::ownOrderMatched, this,
        &OwnOrdersDataSource::onOwnOrderMatched);
    connect(_orderbook, &OrderbookClient::ownOrderCanceled, this,
        &OwnOrdersDataSource::onOwnOrderCanceled);
    connect(_orderbook, &OrderbookClient::ownOrdersChanged, this,
        &OwnOrdersDataSource::onOwnOrdersChanged);
    setOrders({});
}

//==============================================================================

void OwnOrdersDataSource::fetch()
{
    onOwnOrdersChanged(_pairId);
}

//==============================================================================

const OwnOrdersDataSource::Orders& OwnOrdersDataSource::orders() const
{
    return _orders;
}

//==============================================================================

void OwnOrdersDataSource::onOwnOrderPlaced(OwnOrderEntry entry)
{
    if (_pairId != entry.pairId.toStdString()) {
        return;
    }

    _orders.emplace_back(entry);
    orderAdded(entry);
}

//==============================================================================

void OwnOrdersDataSource::onOwnOrderChanged(OwnOrderEntry entry)
{
    auto it = std::find_if(std::begin(_orders), std::end(_orders),
        [orderId = entry.id](const auto& order) { return orderId == order.id; });

    if (it != std::end(_orders)) {
        *it = entry;
        orderChanged(*it);
    }
}

//==============================================================================

void OwnOrdersDataSource::onOwnOrderMatched(std::string localId)
{
    auto it = std::find_if(std::begin(_orders), std::end(_orders),
        [localId = QString::fromStdString(localId)](
            const auto& order) { return localId == order.id; });

    if (it != std::end(_orders)) {
        auto order = *it;
        _orders.erase(it);
        orderRemoved(order);
    }
}

//==============================================================================

void OwnOrdersDataSource::onOwnOrderCanceled(std::string orderId)
{
    onOwnOrderMatched(orderId);
}

//==============================================================================

void OwnOrdersDataSource::onOwnOrdersChanged(std::string pairId)
{
    if (_pairId != pairId) {
        return;
    }

    QPointer<OwnOrdersDataSource> self(this);

    Promise<Orders>([orderbook = _orderbook](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(orderbook, [=] {
            Orders result;

            auto traverser = [&](auto order) {
                auto parsed = ParseOrder(order);
                result.emplace_back(parsed);
            };

            orderbook->ownOrders(std::bind(traverser, std::placeholders::_1));
            resolve(result);
        });
    })
        .then([self](Orders orders) {
            if (!self) {
                return;
            }

            self->setOrders(orders);
        });
}

//==============================================================================

void OwnOrdersDataSource::setOrders(OwnOrdersDataSource::Orders newOrders)
{
    _orders.swap(newOrders);
    fetched();
}

//==============================================================================
