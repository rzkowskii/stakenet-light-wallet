#ifndef ORDERBOOKBESTPRICEMODEL_HPP
#define ORDERBOOKBESTPRICEMODEL_HPP

#include <QObject>
#include <array>

#include <Orderbook/TradingOrdersModel.hpp>
#include <Orderbook/Types.hpp>
#include <queue>

namespace orderbook {

class OrderbookBestPriceModel : public QObject {
    Q_OBJECT
public:
    explicit OrderbookBestPriceModel(
        TradingOrdersModel* ordersModel, bool isBuy, QObject* parent = nullptr);
    virtual ~OrderbookBestPriceModel();

    int64_t bestPriceByPairId(std::string pairId) const;

private slots:
    void onOrderAdded(std::string pairId, TradingOrdersModel::Details details, bool isAsk);
    void onOrderRemoved(std::string pairId, TradingOrdersModel::Details details, bool isAsk);
    void onOrdersChanged(std::string pairId);

private:
    void init();

private:
    using Heap = std::multiset<int64_t, std::function<bool(int64_t, int64_t)>>;
    struct Price {
        int64_t value{ 0 };
    };

    Heap& makeHeap(std::string pairId, std::vector<int64_t> data = {});
    void updateBestPrice(std::string pairId);

private:
    TradingOrdersModel* _ordersModel{ nullptr };
    std::map<std::string, Heap> _dynamics;
    std::map<std::string, Price> _bestPrice;
    bool _isAsk{ false };
};
}

#endif
