#ifndef ORDERBOOKREFUNDABLEFEES_HPP
#define ORDERBOOKREFUNDABLEFEES_HPP

#include <QObject>
#include <vector>

#include <Orderbook/OrderbookApiClient.hpp>
#include <Orderbook/Protos/stakenet/orderbook/commands.pb.h>

namespace orderbook {

class OrderbookRefundableFees : public QObject {
    Q_OBJECT
public:
    using RefundFeeResponse = io::stakenet::orderbook::protos::RefundFeeResponse;
    using RefundableFeePayment = std::tuple<std::vector<unsigned char>, int64_t>;
    using ConnextPaymentInfoResponse
        = io::stakenet::orderbook::protos::GetConnextPaymentInformationResponse;
    using ConnextChannelContractDeploymentFeeResponse
        = io::stakenet::orderbook::protos::GetConnextChannelContractDeploymentFeeResponse;
    using RegisterConnextChannelContractDeploymentFeeResponse
        = io::stakenet::orderbook::protos::RegisterConnextChannelContractDeploymentFeeResponse;
    explicit OrderbookRefundableFees(OrderbookApiClient& client, QObject* parent = nullptr);

    Promise<RefundFeeResponse> refundFee(
        std::string currency, std::vector<RefundableFeePayment> fees);
    Promise<int64_t> refundableAmount(std::string currency, std::vector<RefundableFeePayment> fees);
    Promise<std::pair<std::string, std::string>> getLndFeePaymentRequest(
        std::string currency, int64_t quantity);
    Promise<ConnextPaymentInfoResponse> getConnextFeePaymentHash(std::string currency);
    Promise<ConnextChannelContractDeploymentFeeResponse> getConnextContractDeploymentFeeResponse();
    Promise<RegisterConnextChannelContractDeploymentFeeResponse>
    registerConnextContractDeploymentFeeResponse(std::string txHash);

signals:

private:
    OrderbookApiClient& _client;
};
}

#endif // ORDERBOOKREFUNDABLEFEES_HPP
