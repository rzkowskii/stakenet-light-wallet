#include "OrderbookRefundableFees.hpp"
#include <Orderbook/OrderbookClient.hpp>

namespace orderbook {

//==============================================================================

OrderbookRefundableFees::OrderbookRefundableFees(OrderbookApiClient& client, QObject* parent)
    : QObject(parent)
    , _client(client)
{
}

//==============================================================================

Promise<OrderbookRefundableFees::RefundFeeResponse> OrderbookRefundableFees::refundFee(
    std::string currency, std::vector<RefundableFeePayment> fees)
{
    Command refundFeeCmd;
    auto& cmd = *refundFeeCmd.mutable_refundfeecommand();
    cmd.set_currency(currency);
    for (auto&& fee : fees) {
        auto& refundedFee = *cmd.add_refundedfees();
        refundedFee.set_allocated_paidamount(ConvertToBigInt(std::get<1>(fee)).release());
        const auto& paymentHash = std::get<0>(fee);
        refundedFee.set_paymenthash(std::string(paymentHash.begin(), paymentHash.end()));
    }
    return _client.makeRequest(refundFeeCmd).then([](OrderbookResponse response) {
        return response.refundfeeresponse();
    });
}

//==============================================================================

Promise<int64_t> OrderbookRefundableFees::refundableAmount(
    std::string currency, std::vector<RefundableFeePayment> fees)
{
    Command getRefundableAmountCmd;
    auto& cmd = *getRefundableAmountCmd.mutable_getrefundableamountcommand();
    cmd.set_currency(currency);
    for (auto&& fee : fees) {
        auto& payment = *cmd.add_refundablepayments();
        payment.set_allocated_paidamount(ConvertToBigInt(std::get<1>(fee)).release());
        const auto& paymentHash = std::get<0>(fee);
        payment.set_paymenthash(std::string(paymentHash.begin(), paymentHash.end()));
    }
    return _client.makeRequest(getRefundableAmountCmd).then([](OrderbookResponse response) {
        return ConvertBigInt(response.getrefundableamountresponse().amount());
    });
}

//==============================================================================

Promise<std::pair<std::string, std::string>> OrderbookRefundableFees::getLndFeePaymentRequest(
    std::string currency, int64_t quantity)
{
    Command getFeePaymentCommand;
    auto cmd = new io::stakenet::orderbook::protos::GetLndPaymentInvoiceCommand;
    cmd->set_currency(currency);
    cmd->set_allocated_amount(ConvertToBigInt(quantity).release());
    getFeePaymentCommand.set_allocated_getlndpaymentinvoicecommand(cmd);
    return _client.makeRequest(getFeePaymentCommand).then([](OrderbookResponse response) {
        auto r = response.getlndpaymentinvoiceresponse();
        return std::make_pair(r.currency(), r.nofeerequired() ? std::string{} : r.paymentrequest());
    });
}

//==============================================================================

Promise<OrderbookRefundableFees::ConnextPaymentInfoResponse>
OrderbookRefundableFees::getConnextFeePaymentHash(std::string currency)
{
    Command getConnextPaymentInformationCommand;
    auto cmd = new io::stakenet::orderbook::protos::GetConnextPaymentInformationCommand;
    cmd->set_currency(currency);
    getConnextPaymentInformationCommand.set_allocated_getconnextpaymentinformationcommand(cmd);
    return _client.makeRequest(getConnextPaymentInformationCommand)
        .then([](OrderbookResponse response) {
            return response.getconnextpaymentinformationresponse();
        });
}

//==============================================================================

Promise<OrderbookRefundableFees::ConnextChannelContractDeploymentFeeResponse>
OrderbookRefundableFees::getConnextContractDeploymentFeeResponse()
{
    Command GetConnextChannelContractDeploymentFeeCommand;
    auto cmd = new io::stakenet::orderbook::protos::GetConnextChannelContractDeploymentFeeCommand;
    GetConnextChannelContractDeploymentFeeCommand
        .set_allocated_getconnextchannelcontractdeploymentfeecommand(cmd);
    return _client.makeRequest(GetConnextChannelContractDeploymentFeeCommand)
        .then([](OrderbookResponse response) {
            return response.getconnextchannelcontractdeploymentfeeresponse();
        });
}

//==============================================================================

Promise<OrderbookRefundableFees::RegisterConnextChannelContractDeploymentFeeResponse>
OrderbookRefundableFees::registerConnextContractDeploymentFeeResponse(std::string txHash)
{
    Command RegisterConnextChannelContractDeploymentFeeCommand;
    auto cmd
        = new io::stakenet::orderbook::protos::RegisterConnextChannelContractDeploymentFeeCommand;
    cmd->set_transactionhash(txHash);
    RegisterConnextChannelContractDeploymentFeeCommand
        .set_allocated_registerconnextchannelcontractdeploymentfeecommand(cmd);
    return _client.makeRequest(RegisterConnextChannelContractDeploymentFeeCommand)
        .then([](OrderbookResponse response) {
            return response.registerconnextchannelcontractdeploymentfeeresponse();
        });
}

//==============================================================================
}
