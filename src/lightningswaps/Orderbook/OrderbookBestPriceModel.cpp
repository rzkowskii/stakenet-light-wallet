#include "OrderbookBestPriceModel.hpp"
#include "OrderbookClient.hpp"

namespace orderbook {

//==============================================================================

OrderbookBestPriceModel::OrderbookBestPriceModel(
    TradingOrdersModel* ordersModel, bool isBuy, QObject* parent)
    : QObject(parent)
    , _ordersModel(ordersModel)
    , _isAsk(!isBuy)
{
    init();
}

//==============================================================================

OrderbookBestPriceModel::~OrderbookBestPriceModel() {}

//==============================================================================

int64_t OrderbookBestPriceModel::bestPriceByPairId(std::string pairId) const
{
    if (_dynamics.count(pairId) == 0) {
        return 0;
    }

    return _bestPrice.count(pairId) > 0 ? _bestPrice.at(pairId).value : 0;
}

//==============================================================================

void OrderbookBestPriceModel::onOrderAdded(
    std::string pairId, TradingOrdersModel::Details details, bool isAsk)
{
    if (_isAsk != isAsk) {
        return;
    }

    auto& heap = makeHeap(pairId);
    heap.insert(details.price);
    updateBestPrice(pairId);
}

//==============================================================================

void OrderbookBestPriceModel::onOrderRemoved(
    std::string pairId, TradingOrdersModel::Details details, bool isAsk)
{
    if (_isAsk != isAsk || _dynamics.count(pairId) == 0) {
        return;
    }

    auto& heap = _dynamics.at(pairId);
    heap.erase(details.price);
    updateBestPrice(pairId);
}

//==============================================================================

void OrderbookBestPriceModel::onOrdersChanged(std::string pairId)
{
    std::vector<int64_t> data;
    auto transform = [&data](const auto& from) {
        data.resize(from.size());
        std::transform(std::begin(from), std::end(from), std::begin(data),
            [](const auto& it) { return it.second.price; });
    };

    if (_isAsk) {
        transform(_ordersModel->askOrders(pairId));
    } else {
        transform(_ordersModel->bidOrders(pairId));
    }

    makeHeap(pairId, std::move(data));
    updateBestPrice(pairId);
}

//==============================================================================

void OrderbookBestPriceModel::init()
{
    connect(_ordersModel, &TradingOrdersModel::orderAdded, this,
        &OrderbookBestPriceModel::onOrderAdded);
    connect(_ordersModel, &TradingOrdersModel::orderRemoved, this,
        &OrderbookBestPriceModel::onOrderRemoved);
    connect(_ordersModel, &TradingOrdersModel::ordersChanged, this,
        &OrderbookBestPriceModel::onOrdersChanged);
}

//==============================================================================

OrderbookBestPriceModel::Heap& OrderbookBestPriceModel::makeHeap(
    std::string pairId, std::vector<int64_t> data)
{
    if (_isAsk) {
        _dynamics.emplace(pairId, Heap{ std::begin(data), std::end(data), std::less<int64_t>{} });
    } else {
        _dynamics.emplace(
            pairId, Heap{ std::begin(data), std::end(data), std::greater<int64_t>{} });
    }

    return _dynamics.at(pairId);
}

//==============================================================================

void OrderbookBestPriceModel::updateBestPrice(std::string pairId)
{
    auto& price = _bestPrice[pairId];
    if (_dynamics.count(pairId) > 0) {
        auto d = _dynamics.at(pairId);
        if (!d.empty()) {
            // if we have dynamics, we will update from it
            price.value = *d.cbegin();
            return;
        }
    }

    price.value = 0;
}

//==============================================================================
}
