#ifndef CHANNELRENTING_HPP
#define CHANNELRENTING_HPP

#include <Orderbook/OrderbookApiClient.hpp>
#include <QObject>

namespace orderbook {

class OrderbookChannelRenting : public QObject {
    Q_OBJECT
public:
    using RentResponse = io::stakenet::orderbook::protos::RentChannelResponse;
    using RentInvoice = std::string;
    using RentChannelStatusResponse = io::stakenet::orderbook::protos::GetChannelStatusResponse;
    using RentingFeeResponse = io::stakenet::orderbook::protos::GetFeeToRentChannelResponse;
    using GeneratePaymentHashToRentChannelResponse = io::stakenet::orderbook::protos::GeneratePaymentHashToRentChannelResponse;

    using PaymentHashToExtendChannel = std::string;
    using InvoiceToExtendRentedChannel = std::string;
    using ExtendRentedChannelTimeResponse
        = io::stakenet::orderbook::protos::ExtendRentedChannelTimeResponse;
    using PaymentHash = std::string;

    explicit OrderbookChannelRenting(OrderbookApiClient& client, QObject* parent = nullptr);

    Promise<RentingFeeResponse> feeToRentChannel(std::string currency, std::string payingCurrency,
        int64_t capacity, int64_t lifetimeSeconds);

    Promise<RentInvoice> generateRentInvoice(std::string currency, std::string payingCurrency,
        int64_t capacity, int64_t lifetimeSeconds);

    Promise<PaymentHash> generatePaymentHashToRentChannel(std::string currency, std::string payingCurrency,
                                                          int64_t capacity, int64_t lifetimeSeconds);

    Promise<RentResponse> rent(std::string paymentHash, std::string payingCurrency);

    Promise<RentChannelStatusResponse> getRentChannelStatus(std::string channelId);

    Promise<InvoiceToExtendRentedChannel> generateInvoiceToExtendRentedChannel(
        std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds);

    Promise<PaymentHashToExtendChannel> generatePaymentHashToExtendConnextRentedChannel(
        std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds);

    Promise<ExtendRentedChannelTimeResponse> extendRentedChannelTime(
        std::string paymentHash, std::string payingCurrency);

    Promise<int64_t> feeToExtendedRentChannel(
        std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds);

signals:

private:
    OrderbookApiClient& _client;
};
}

#endif // CHANNELRENTING_HPP
