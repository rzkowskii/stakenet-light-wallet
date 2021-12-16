#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include <Protos/lssdrpc.grpc.pb.h>
#include <grpcpp/grpcpp.h>

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::CompletionQueue;
using grpc::Status;

using namespace std;

//==============================================================================

static std::unique_ptr<lssdrpc::BigInteger> ConvertToBigInt(int64_t value)
{
    std::unique_ptr<lssdrpc::BigInteger> result(new lssdrpc::BigInteger);
    result->set_value(std::to_string(value));
    return result;
}

//==============================================================================

static std::string OrderToString(lssdrpc::Order order)
{
    std::stringstream ss;
    ss << "OrderId: " << order.orderid() << '\n'
       << "Quantity: " << order.funds().value() << '\n'
       << "Open: [";
    for (auto&& open : order.open()) {
        ss << "{" << open.orderid() << ", " << open.amount().value() << "}";
    }
    ss << "]" << '\n';
    ss << "Closed: [";

    for (auto&& closed : order.closed()) {
        ss << "{" << closed.orderid() << ", " << closed.amount().value() << "}";
    }
    ss << "]" << '\n';

    return ss.str();
}

//==============================================================================

class LightningSwapsClient {
public:
    LightningSwapsClient(std::shared_ptr<Channel> channel)
        : _swaps(lssdrpc::swaps::NewStub(channel))
        , _currencies(lssdrpc::currencies::NewStub(channel))
        , _orders(lssdrpc::orders::NewStub(channel))
        , _pairs(lssdrpc::tradingPairs::NewStub(channel))
    {
    }

    void subscribeSwaps(std::function<void(lssdrpc::SwapResult)> onCallback)
    {
        lssdrpc::SubscribeSwapsRequest request;

        ClientContext context;
        std::unique_ptr<ClientReader<lssdrpc::SwapResult>> reader(
            _swaps->SubscribeSwaps(&context, request));

        lssdrpc::SwapResult reply;
        while (reader->Read(&reply)) {
            onCallback(reply);
        }

        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "GetSubscribeSwaps rpc succeeded." << std::endl;
        } else {
            std::cout << "GetSubscribeSwaps rpc failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void subscribeOrders()
    {
        lssdrpc::SubscribeOrdersRequest request;

        ClientContext context;
        std::unique_ptr<ClientReader<lssdrpc::OrderUpdate>> reader(
            _orders->SubscribeOrders(&context, request));

        lssdrpc::OrderUpdate reply;

        while (reader->Read(&reply)) {
            if (reply.update_case() == lssdrpc::OrderUpdate::UpdateCase::kOrderAdded) {
                std::cout << "Order added: " << reply.orderadded().funds().value() << std::endl;
            } else {
                std::cout << "Order removed: " << reply.orderremoval().funds().value() << std::endl;
            }
        }

        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "SubscribeOrders rpc succeeded." << std::endl;
        } else {
            std::cout << "SubscribeOrders rpc failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void subscribeOwnOrders()
    {
        lssdrpc::SubscribeOrdersRequest request;

        ClientContext context;
        std::unique_ptr<ClientReader<lssdrpc::OwnOrderUpdate>> reader(
            _orders->SubscribeOwnOrders(&context, request));

        lssdrpc::OwnOrderUpdate reply;

        while (reader->Read(&reply)) {
            if (reply.update_case() == lssdrpc::OwnOrderUpdate::UpdateCase::kOrderAdded) {
                std::cout << "Own order added: " << OrderToString(reply.orderadded()) << std::endl;
            } else if (reply.update_case() == lssdrpc::OwnOrderUpdate::UpdateCase::kOrderChanged) {
                std::cout << "Own order changed: " << OrderToString(reply.orderchanged())
                          << std::endl;
            } else {
                std::cout << "Own Order completed: " << reply.ordercompleted() << std::endl;
            }
        }

        Status status = reader->Finish();
        if (status.ok()) {
            std::cout << "SubscribeOwnOrders rpc succeeded." << std::endl;
        } else {
            std::cout << "SubscribeOwnOrders rpc failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void addCurrency(std::string currency, std::string tlscert, std::string rpcchannel)
    {
        lssdrpc::AddCurrencyRequest request;
        request.set_currency(currency);
        request.mutable_lnd()->set_certpath(tlscert);
        request.mutable_lnd()->set_lndchannel(rpcchannel);

        lssdrpc::AddCurrencyResponse reply;
        ClientContext context;

        auto status = _currencies->AddCurrency(&context, request, &reply);

        if (status.ok()) {
            std::cout << "AddCurrency rpc succeeded." << std::endl;
        } else {
            std::cout << "AddCurrency rpc failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void activatePair(std::string pair)
    {
        lssdrpc::EnableTradingPairRequest request;
        request.set_pairid(pair);

        lssdrpc::EnableTradingPairResponse reply;
        ClientContext context;
        auto status = _pairs->EnableTradingPair(&context, request, &reply);

        if (status.ok()) {
            std::cout << "ActivatePair rpc succeeded." << std::endl;
        } else {
            std::cout << "ActivatePair rpc failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void placeOrder(std::string pairId, int64_t price, uint64_t quantity, lssdrpc::OrderSide side)
    {
        lssdrpc::PlaceOrderRequest request;
        request.set_allocated_price(ConvertToBigInt(price).release());
        request.set_allocated_funds(ConvertToBigInt(quantity).release());
        request.set_side(side);
        request.set_pairid(pairId);

        lssdrpc::PlaceOrderResponse reply;
        ClientContext context;

        auto status = _orders->PlaceOrder(&context, request, &reply);

        if (status.ok()) {
            std::cout << "PlaceOrder rpc call succeeded " << std::endl;

            if (reply.has_order()) {
                std::cout << "PlaceOrder rpc call response orderId: " << reply.order().orderid()
                          << " pairId: " << reply.order().pairid() << std::endl;
            }

            if (reply.has_failure()) {
                if (reply.failure().has_swapfailure()) {
                    std::cout << "PlaceOrder rpc call response failurereason: "
                              << reply.failure().swapfailure().failurereason() << std::endl;
                } else {
                    std::cout << "PlaceOrder rpc call response failurereason: "
                              << reply.failure().orderbookfailure().failurereason() << std::endl;
                }
            }

        } else {
            std::cout << "PlaceOrder rpc failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void listOrders(std::string pairId, bool includeOwnOrders)
    {
        lssdrpc::ListOrdersRequest request;
        request.set_pairid(pairId);
        //        request.set_includeownorders(includeOwnOrders);
        //        request.set_skip(0);
        request.set_limit(100);

        lssdrpc::ListOrdersResponse reply;
        ClientContext context;

        auto status = _orders->ListOrders(&context, request, &reply);

        if (status.ok()) {
            std::cout << "ListOrders rpc succeeded. Orders size: " << reply.orders_size()
                      << std::endl;
        } else {
            std::cout << "ListOrders rpc failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

    void cancelOrder(std::string pairId, std::string orderId)
    {
        lssdrpc::CancelOrderRequest request;
        request.set_pairid(pairId);
        request.set_orderid(orderId);

        lssdrpc::CancelOrderResponse reply;
        ClientContext context;

        auto status = _orders->CancelOrder(&context, request, &reply);

        if (status.ok()) {
            std::cout << "CancelOrder rpc succeeded." << std::endl;
        } else {
            std::cout << "CancelOrder rpc failed." << std::endl;
            std::cout << status.error_code() << ": " << status.error_message() << std::endl;
        }
    }

private:
    std::unique_ptr<lssdrpc::swaps::Stub> _swaps;
    std::unique_ptr<lssdrpc::currencies::Stub> _currencies;
    std::unique_ptr<lssdrpc::orders::Stub> _orders;
    std::unique_ptr<lssdrpc::tradingPairs::Stub> _pairs;
};

static void HandleCommand(LightningSwapsClient& client, QString command, QStringList args)
{
    if (command == "placeorder") {
        if (args.size() != 4) {
            throw std::runtime_error("Placeorder invalid args count, expect 4");
        }

        lssdrpc::OrderSide orderSide
            = args.at(3).toInt() == 0 ? lssdrpc::OrderSide::buy : lssdrpc::OrderSide::sell;
        client.placeOrder(
            args.at(0).toStdString(), args.at(1).toDouble(), args.at(2).toInt(), orderSide);
    } else if (command == "addcurrency") {
        if (args.size() != 3) {
            throw std::runtime_error("Addcurrency invalid arg count, expected 3");
        }

        client.addCurrency(
            args.at(0).toUpper().toStdString(), args.at(1).toStdString(), args.at(2).toStdString());
    } else if (command == "activate") {
        if (args.size() != 1) {
            throw std::runtime_error("Activate pair invalid arg count, expected 1");
        }

        client.activatePair(args.at(0).toStdString());
    } else if (command == "subscribeswaps") {
        client.subscribeSwaps([](lssdrpc::SwapResult reply) {
            if (reply.value_case() == lssdrpc::SwapResult::ValueCase::kSuccess) {
                std::cout << "Got swap success" << reply.success().orderid() << std::endl;
            } else {
                std::cout << "Got swap success" << reply.failure().orderid() << std::endl;
            }
        });
    } else if (command == "subscribeorders") {
        client.subscribeOrders();
    } else if (command == "subscribeownorders") {
        client.subscribeOwnOrders();
    } else if (command == "listorders") {
        if (args.size() != 1) {
            throw std::runtime_error("Listorders invalid arg count, expected 1");
        }
        client.listOrders((args.at(0).toStdString()), true);
    } else if (command == "cancelorder") {
        if (args.size() != 2) {
            throw std::runtime_error("Cancelorder invalid arg count, expected 2");
        }

        client.cancelOrder(args.at(0).toStdString(), args.at(1).toStdString());
    }
}

int main(int argc, char** argv)
{
    QCoreApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption portOpt("port", "Number of port", "Port number");

    auto args = app.arguments();
    args.takeFirst();
    args.erase(std::remove_if(std::begin(args), std::end(args),
                   [](const QString& arg) { return arg.startsWith('-'); }),
        std::end(args));

    if (args.empty()) {
        std::cout << "No command specified" << std::endl;
        return 0;
    }

    auto command = args.takeFirst();

    parser.addOption(portOpt);

    parser.process(app);

    QString channel("localhost:");
    channel.append(parser.isSet(portOpt) ? parser.value(portOpt) : QString("50051"));

    LightningSwapsClient swapsClient(
        grpc::CreateChannel(channel.toStdString(), grpc::InsecureChannelCredentials()));

    HandleCommand(swapsClient, command, args);

    return 0;
}
