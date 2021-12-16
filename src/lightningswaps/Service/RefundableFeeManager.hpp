#ifndef REFUNDABLEFEEMANAGEMENT_HPP
#define REFUNDABLEFEEMANAGEMENT_HPP

#include <Orderbook/Types.hpp>
#include <Service/RefundableFeeManagerState.hpp>
#include <Utils/Utils.hpp>

#include <QObject>

namespace orderbook {
class OrderbookClient;
class OrderbookRefundableFees;
class PaymentNodeRegistrationService;
}

namespace Utils {
class LevelDBSharedDatabase;
template <class T> class GenericProtoDatabase;
}

namespace swaps {

class AbstractSwapClientPool;
class AbstractSwapClient;

//==============================================================================

/*!
 * \brief The FeeFailure struct describes error that happens during paying fees
 */
struct FeeFailure {
    enum class Reason { RemoteError, NoRouteFound, NotEnoughBalance, UnknownReason };

    explicit FeeFailure(Reason reason, std::string message = {});

    std::string message;
    Reason reason{ Reason::UnknownReason };
};

//==============================================================================

class RefundableFeeManager : public QObject {
    Q_OBJECT
public:
    struct RentingPaymentFeeResponse {
        //        std::string pubkey;
        std::string paymentHash;
        storage::RefundableFee fee;
    };

    struct ExtendPaymentFeeResponse {
        std::string paymentHash;
        storage::RefundableFee fee;
    };

    explicit RefundableFeeManager(const AbstractSwapClientPool& clients,
        std::shared_ptr<Utils::LevelDBSharedDatabase> sharedDb,
        QPointer<orderbook::OrderbookClient> client,
        QPointer<orderbook::PaymentNodeRegistrationService> registrationService,
        QObject* parent = nullptr);
    ~RefundableFeeManager();

    Promise<storage::RefundableFee> payPlaceOrderFee(
        const orderbook::TradingPair& tradingPair, int64_t quantity, bool isBuy);

    Promise<RentingPaymentFeeResponse> payChannelRentingFee(std::string requestedCurrency,
        std::string payingCurrency, std::string paymentRequest, int64_t initialAmount);
    Promise<ExtendPaymentFeeResponse> payExtendTimeRentingFee(
        std::string payingCurrency, std::string paymentRequest);

    Promise<void> refundFee(const storage::RefundableFee& fee);

    void burnRefundableFee(const std::vector<uint64_t>& ids);
    void trackPlacedOrderFee(uint64_t id, std::string orderId);
    boost::optional<storage::RefundableFee> feeByOrderId(const std::string& orderId);
    RefundableFeeManagerState* state() const;

public:
    static int64_t CalculatePlaceOrderFee(
        const orderbook::TradingPair& tradingPair, int64_t quantity, bool isBuy);

signals:
    void refundableFeeAdded(storage::RefundableFee fee);
    void refundableFeeBurnt(uint64_t id);

private slots:
    void onRefundableAmountChanged(
        std::string currency, const RefundableFeeManagerState::RefundableAmount& amount);

private:
    using ExtendPaymentResponse = std::tuple<std::string, int64_t>; // (payment hash, amount)
    Promise<std::string> executeLndPlaceOrderFeePayment(
        std::string currency, std::string paymentRequest);
    Promise<std::string> executeConnextPlaceOrderFeePayment(
        std::string currency, int64_t fee, std::string publicIdentifier, std::string paymentHash);
    Promise<RentingPaymentFeeResponse> executeRentingPayment(
        std::string requestedCurrency, std::string payingCurrency, std::string paymentRequest, int64_t initialAmount);
    Promise<ExtendPaymentResponse> executeExtendRentedChannelPayment(
        std::string payingCurrency, std::string paymentRequest);
    void onHandleRefundError(QString errorMsg, std::string currency);
    Promise<void> ensurePaymentNodeRegistered(std::string currency);

    Promise<storage::RefundableFee> payLndPlaceOrderFee(
        const orderbook::TradingPair& tradingPair, int64_t quantity, bool isBuy);
    Promise<storage::RefundableFee> payConnextPlaceOrderFee(
        const orderbook::TradingPair& tradingPair, int64_t quantity, bool isBuy);

private:
    using RefundingDb = Utils::GenericProtoDatabase<storage::RefundableFee>;
    const swaps::AbstractSwapClientPool& _swapClients;
    std::unique_ptr<RefundingDb> _refundableFeeDb;
    QPointer<orderbook::OrderbookClient> _orderbook;
    QPointer<RefundableFeeManagerState> _state;
    QPointer<orderbook::PaymentNodeRegistrationService> _paymentNodeRegistration;
    std::unordered_map<std::string, uint64_t> _placedOrders;
};

//==============================================================================
}

#endif // REFUNDABLEFEEMANAGEMENT_HPP
