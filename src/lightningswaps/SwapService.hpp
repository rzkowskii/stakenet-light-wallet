#ifndef SWAPSERVICE_HPP
#define SWAPSERVICE_HPP

#include <Orderbook/Types.hpp>
#include <Service/RefundableFeeManager.hpp>
#include <Swaps/Types.hpp>
#include <Utils/Utils.hpp>

#include <QObject>
#include <QUrl>
#include <memory>

class LndGrpcClient;

namespace orderbook {
class OrderbookClient;
class OrderbookSwapPeerPool;
class OrderbookBestPriceModel;
class ChannelRentingManager;
class PaymentNodeRegistrationService;
}

namespace swaps {

class SwapManager;
class SwapPeerPool;
class SwapClientPool;
class AbstractSwapRepository;
class AbstractSwapClientFactory;

using PlaceOrderResult = boost::variant<SwapSuccess, orderbook::OwnOrder>;

/*!
 * Error that will be returnes as result of placeOrder function call
 */
using PlaceOrderFailure = boost::variant<SwapFailure, FeeFailure>;

class SwapService : public QObject {
    Q_OBJECT
public:
    enum class State { Connected, Disconnected, InMaintenance };

    struct ConnextConfig {
        QString host;
        int port{ 0 };
    };

    struct Config {
        QString dataDirPath;
        QUrl orderbookUrl;
        QString botMakerSecret;
        QString clientId;
        bool payFee{ true };
        ConnextConfig connextConfig;

        swaps::AbstractSwapClientFactory* swapClientFactory{ nullptr };

        static QUrl DefaultOrderbookUrl();
        static QUrl StagingOrderbookUrl();
    };

    explicit SwapService(Config cfg, QObject* parent = nullptr);
    ~SwapService();

    void start();
    void stop();

    Promise<void> addCurrency(std::string currency);
    Promise<void> activatePair(std::string pairId);
    Promise<void> deactivatePair(std::string pairId);
    Promise<std::vector<std::string>> activatedPairs();
    Promise<std::vector<orderbook::TradingPair>> tradingPairs();
    Promise<std::vector<orderbook::LimitOrder>> listOrders(
        std::string pairId, int64_t lastKnownPrice = 0, uint32_t limit = 100);
    Promise<std::vector<orderbook::OwnOrder>> listOwnOrders(std::string pairId);
    Promise<std::vector<std::string>> listCurrencies();
    Promise<PlaceOrderResult> placeOrder(orderbook::LimitOrder ownOrder);
    Promise<void> cancelOrder(std::string pairId, std::string localId);
    void cancelAllOrders(std::string pairId);

    orderbook::OrderbookClient* orderBookClient() const;
    AbstractSwapRepository* swapRepository() const;
    RefundableFeeManager* feeRefunding() const;
    orderbook::ChannelRentingManager* channelRentingManager() const;

signals:
    void stateChanged(State newState);
    void swapExecuted(SwapSuccess swap);
    void swapFailed(SwapFailure swap);

private slots:
    void onCheckState();
    void onSwapSuccess(const SwapSuccess& swap, const storage::RefundableFee& fee);
    void onSwapFailure(const SwapFailure& failure, const storage::RefundableFee& fee);

private:
    void init();
    Promise<void> executeBaseOrderChecks(const orderbook::LimitOrder& ownLimitOrder);
    Promise<SwapSuccess> executeSwapHelper(orderbook::PeerOrder maker, orderbook::OwnOrder taker);
    Promise<PlaceOrderResult> executePlaceOrder(
        const orderbook::LimitOrder& ownLimitOrder, const storage::RefundableFee& fee);
    Promise<storage::RefundableFee> executePlaceOrderFeePayment(
        const orderbook::LimitOrder& orderbookOrder);
    Promise<void> verifyCanSendRecv(const orderbook::LimitOrder& ownOrder);
    Promise<PlaceOrderResult> placePartialOrder(const orderbook::OwnOrder& baseOrder,
        int64_t orderPortion, const storage::RefundableFee& fee);
    Promise<PlaceOrderResult> placeOrder(
        const orderbook::OwnLimitOrder& order, const storage::RefundableFee& fee);
    Promise<PlaceOrderResult> handlePlaceOrderResult(
        orderbook::PlaceOrderOutcome outcome, storage::RefundableFee fee);
    void handlePlaceOrderFailure(const storage::RefundableFee& fee);

private:
    Config _cfg;
    std::vector<orderbook::TradingPair> _tradingPairs;
    RefundableFeeManager* _refundableFees{ nullptr };
    orderbook::ChannelRentingManager* _rentingManager{ nullptr };
    SwapClientPool* _clientsPool{ nullptr };
    AbstractSwapRepository* _repository{ nullptr };
    orderbook::OrderbookClient* _orderbook{ nullptr };
    orderbook::OrderbookSwapPeerPool* _peersPool{ nullptr };
    SwapManager* _swapManager{ nullptr };
    orderbook::OrderbookBestPriceModel* _buyBestPriceModel{ nullptr };
    orderbook::OrderbookBestPriceModel* _sellBestPriceModel{ nullptr };
    orderbook::PaymentNodeRegistrationService* _paymentNodeRegistration{ nullptr };
};
}

#endif // SWAPSERVICE_HPP
