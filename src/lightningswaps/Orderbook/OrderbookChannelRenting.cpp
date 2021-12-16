#include "OrderbookChannelRenting.hpp"
#include <Orderbook/OrderbookClient.hpp>

namespace orderbook {

//==============================================================================

OrderbookChannelRenting::OrderbookChannelRenting(OrderbookApiClient& client, QObject* parent)
    : QObject(parent)
    , _client(client)
{
}

//==============================================================================

Promise<OrderbookChannelRenting::RentingFeeResponse> OrderbookChannelRenting::feeToRentChannel(
    std::string currency, std::string payingCurrency, int64_t capacity, int64_t lifetimeSeconds)
{
    Command getfeeToRentChannelCommand;
    io::stakenet::orderbook::protos::GetFeeToRentChannelCommand& cmd
        = *getfeeToRentChannelCommand.mutable_getfeetorentchannelcommand();
    cmd.set_currency(currency);
    cmd.set_payingcurrency(payingCurrency);
    cmd.set_allocated_capacity(ConvertToBigInt(capacity).release());
    cmd.set_lifetimeseconds(lifetimeSeconds);

    return _client.makeRequest(getfeeToRentChannelCommand).then([](OrderbookResponse response) {
        return response.getfeetorentchannelresponse();
    });
}

//==============================================================================

Promise<OrderbookChannelRenting::RentInvoice> OrderbookChannelRenting::generateRentInvoice(
    std::string currency, std::string payingCurrency, int64_t capacity, int64_t lifetimeSeconds)
{
    Command generateRentInvoiceCommand;
    io::stakenet::orderbook::protos::GenerateInvoiceToRentChannelCommand& cmd
        = *generateRentInvoiceCommand.mutable_generateinvoicetorentchannelcommand();
    cmd.set_currency(currency);
    cmd.set_payingcurrency(payingCurrency);
    cmd.set_allocated_capacity(ConvertToBigInt(capacity).release());
    cmd.set_lifetimeseconds(lifetimeSeconds);

    return _client.makeRequest(generateRentInvoiceCommand).then([](OrderbookResponse response) {
        return response.generateinvoicetorentchannelresponse().paymentrequest();
    });
}

//==============================================================================

Promise<OrderbookChannelRenting::PaymentHash> OrderbookChannelRenting::generatePaymentHashToRentChannel(std::string currency, std::string payingCurrency, int64_t capacity, int64_t lifetimeSeconds)
{
    Command generatePaymentHashToRentChannelCommand;
    io::stakenet::orderbook::protos::GeneratePaymentHashToRentChannelCommand& cmd
        = *generatePaymentHashToRentChannelCommand.mutable_generatepaymenthashtorentchannelcommand();
    cmd.set_currency(currency);
    cmd.set_payingcurrency(payingCurrency);
    cmd.set_allocated_capacity(ConvertToBigInt(capacity).release());
    cmd.set_lifetimeseconds(lifetimeSeconds);

    return _client.makeRequest(generatePaymentHashToRentChannelCommand).then([](OrderbookResponse response) {
        return response.generatepaymenthashtorentchannelresponse().paymenthash();
    });
}

//==============================================================================

Promise<OrderbookChannelRenting::RentResponse> OrderbookChannelRenting::rent(
    std::string paymentHash, std::string payingCurrency)
{
    Command rentChannelCommand;
    io::stakenet::orderbook::protos::RentChannelCommand& cmd
        = *rentChannelCommand.mutable_rentchannelcommand();
    cmd.set_paymenthash(QByteArray::fromHex(QByteArray::fromStdString(paymentHash)).toStdString());
    cmd.set_payingcurrency(payingCurrency);
    return _client.makeRequest(rentChannelCommand).then([](OrderbookResponse response) {
        return response.rentchannelresponse();
    });
}

//==============================================================================

Promise<OrderbookChannelRenting::RentChannelStatusResponse>
OrderbookChannelRenting::getRentChannelStatus(std::string channelId)
{
    Command rentChannelStatusCommand;
    io::stakenet::orderbook::protos::GetChannelStatusCommand& cmd
        = *rentChannelStatusCommand.mutable_getchannelstatuscommand();
    cmd.set_channelid(channelId);
    return _client.makeRequest(rentChannelStatusCommand).then([](OrderbookResponse response) {
        return response.getchannelstatusresponse();
    });
}

//==============================================================================

Promise<OrderbookChannelRenting::InvoiceToExtendRentedChannel>
OrderbookChannelRenting::generateInvoiceToExtendRentedChannel(
    std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds)
{
    Command generateInvoiceToExtendRentedChannelCommand;
    io::stakenet::orderbook::protos::GenerateInvoiceToExtendRentedChannelCommand& cmd
        = *generateInvoiceToExtendRentedChannelCommand
               .mutable_generateinvoicetoextendrentedchannelcommand();
    cmd.set_channelid(channelId);
    cmd.set_payingcurrency(payingCurrency);
    cmd.set_lifetimeseconds(lifetimeSeconds);

    return _client.makeRequest(generateInvoiceToExtendRentedChannelCommand)
        .then([](OrderbookResponse response) {
            return response.generateinvoicetoextendrentedchannelresponse().paymentrequest();
        });
}

//==============================================================================

Promise<OrderbookChannelRenting::PaymentHashToExtendChannel>
    OrderbookChannelRenting::generatePaymentHashToExtendConnextRentedChannel(
    std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds)
{
    Command generatePaymentHashToExtendConnextRentedChannelCommand;
    io::stakenet::orderbook::protos::GeneratePaymentHashToExtendConnextRentedChannelCommand& cmd
        = *generatePaymentHashToExtendConnextRentedChannelCommand
               .mutable_generatepaymenthashtoextendconnextrentedchannelcommand();
    cmd.set_channelid(channelId);
    cmd.set_payingcurrency(payingCurrency);
    cmd.set_lifetimeseconds(lifetimeSeconds);

    return _client.makeRequest(generatePaymentHashToExtendConnextRentedChannelCommand)
        .then([](OrderbookResponse response) {
            return response.generatepaymenthashtoextendconnextrentedchannelresponse().paymenthash();
        });
}

//==============================================================================

Promise<OrderbookChannelRenting::ExtendRentedChannelTimeResponse>
OrderbookChannelRenting::extendRentedChannelTime(
    std::string paymentHash, std::string payingCurrency)
{
    Command extendRentedChannelTimeCommand;
    io::stakenet::orderbook::protos::ExtendRentedChannelTimeCommand& cmd
        = *extendRentedChannelTimeCommand.mutable_extendrentedchanneltimecommand();
    cmd.set_paymenthash(QByteArray::fromHex(QByteArray::fromStdString(paymentHash)).toStdString());
    cmd.set_payingcurrency(payingCurrency);
    return _client.makeRequest(extendRentedChannelTimeCommand).then([](OrderbookResponse response) {
        return response.extendrentedchanneltimeresponse();
    });
}

//==============================================================================

Promise<int64_t> OrderbookChannelRenting::feeToExtendedRentChannel(
    std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds)
{
    Command feeToExtendedRentChannelCommand;
    io::stakenet::orderbook::protos::GetFeeToExtendRentedChannelCommand& cmd
        = *feeToExtendedRentChannelCommand.mutable_getfeetoextendrentedchannelcommand();
    cmd.set_channelid(channelId);
    cmd.set_payingcurrency(payingCurrency);
    cmd.set_lifetimeseconds(lifetimeSeconds);

    return _client.makeRequest(feeToExtendedRentChannelCommand)
        .then([](OrderbookResponse response) {
            return ConvertBigInt(response.getfeetoextendrentedchannelresponse().fee());
        });
}

//==============================================================================
}
