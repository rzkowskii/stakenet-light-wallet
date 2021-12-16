#ifndef SWAPSSERVICENOTIFICATIONS_HPP
#define SWAPSSERVICENOTIFICATIONS_HPP

#include <GRPCTools/ServerUtils.hpp>
#include <Orderbook/Types.hpp>
#include <Protos/lssdrpc.grpc.pb.h>
#include <SwapService.hpp>

void FillOrderHelper(lssdrpc::Order& orderToFill, const orderbook::OwnOrder& order);

namespace orderbook {
class OrderbookClient;
}

class SwapsServiceNotifications : public QObject {
    Q_OBJECT
public:
    template <class T> using StreamingChanPtr = std::shared_ptr<qgrpc::StreamingChannel<T>>;

    explicit SwapsServiceNotifications(swaps::SwapService& swapService, QObject* parent = nullptr);

    void addSubscription(StreamingChanPtr<lssdrpc::SwapResult> swapSuccess);
    void addSubscription(StreamingChanPtr<lssdrpc::OrderUpdate> orderUpdate);
    void addSubscription(StreamingChanPtr<lssdrpc::OwnOrderUpdate> ownOrderUpdate);
    void addSubscription(StreamingChanPtr<lssdrpc::OrderbookState> orderbookStateUpdate);

private slots:
    void onSwapSuccessReceived(swaps::SwapSuccess swap);
    void onSwapFailureReceived(swaps::SwapFailure swap);

private:
    void init(swaps::SwapService& swapService);
    void connectSwaps(swaps::SwapService& swapService);
    void connectOrders(orderbook::OrderbookClient* orderbookClient);
    void connectOwnOrders(orderbook::OrderbookClient* orderbookClient);

private:
    std::vector<StreamingChanPtr<lssdrpc::SwapResult>> _swapsSubscriptions;
    std::vector<StreamingChanPtr<lssdrpc::OrderUpdate>> _ordersSubscriptions;
    std::vector<StreamingChanPtr<lssdrpc::OwnOrderUpdate>> _ownOrdersSubscriptions;
    std::vector<StreamingChanPtr<lssdrpc::OrderbookState>> _orderbookStateSubscriptions;
};

#endif // SWAPSSERVICENOTIFICATIONS_HPP
