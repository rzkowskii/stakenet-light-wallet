#include "SwapGRPCServer.hpp"

#include <LssdSwapClientFactory.hpp>
#include <Orderbook/OrderbookChannelRenting.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Utils/Logging.hpp>

#include <grpc++/grpc++.h>

using namespace lssdrpc;

//==============================================================================

static int64_t ConvertBigInt(BigInteger bigInt)
{
    auto result = std::stoll(bigInt.value());
    return result;
}

//==============================================================================

static std::unique_ptr<BigInteger> ConvertToBigInt(int64_t value)
{
    std::unique_ptr<BigInteger> result(new BigInteger);
    result->set_value(std::to_string(value));
    return result;
}

//==============================================================================

SwapGRPCServer::SwapGRPCServer(
    ::swaps::SwapService& swapService, LssdSwapClientFactory& swapClientFactory, uint32_t port)
    : qgrpc::BaseGrpcServer("0.0.0.0:" + std::to_string(port))
    , _swapService(swapService)
    , _swapClientFactory(swapClientFactory)
    , _swapsServiceNotifications(swapService)
{
}

//==============================================================================

void SwapGRPCServer::run()
{
    BaseGrpcServer::run({ &_tradingPairsService, &_ordersService, &_currenciesService,
        &_swapsService, &_rentingService });
}

//==============================================================================

template <class T> using StreamingChanPtr = std::shared_ptr<qgrpc::StreamingChannel<T>>;

void SwapGRPCServer::registerService()
{
    auto tradingPairsService = &_tradingPairsService;
    auto ordersService = &_ordersService;
    auto currenciesService = &_currenciesService;
    auto swapsService = &_swapsService;
    auto rentingService = &_rentingService;

    registerCall(&currencies::AsyncService::RequestAddCurrency, currenciesService,
        [this](auto /*context*/, auto request, auto sender) {
            if (request->has_lnd()) {
                LndClientConfiguration lndClientConf;
                auto conf = request->lnd();
                lndClientConf.channel = conf.lndchannel();

                lndClientConf.tlsCert
                    = conf.tlsCert_case() == LndConfiguration::TlsCertCase::kRawCert
                    ? conf.rawcert()
                    : Utils::ReadCert(QString::fromStdString(conf.certpath()));

                if (conf.macaroon_case() == LndConfiguration::MacaroonCase::kRawMacaroon) {
                    lndClientConf.macaroon = conf.raw_macaroon();
                } else if (conf.macaroon_case() == LndConfiguration::MacaroonCase::kMacaroonPath) {
                    lndClientConf.macaroon
                        = Utils::ReadMacaroon(QString::fromStdString(conf.macaroon_path()))
                              .toStdString();
                }

                _swapClientFactory.registerClientConf(request->currency(), lndClientConf);
            } else if (request->has_connext()) {
                auto conf = request->connext();
                ConnextClientConfiguration connextConf;
                connextConf.channel = conf.connextchannel();
                connextConf.tokenAddress = conf.tokenaddress();
                connextConf.resolverAddress = conf.eventresolver();
                _swapClientFactory.registerConnextConf(request->currency(), connextConf);
            }

            _swapService.addCurrency(request->currency())
                .then([sender] { sender->finish(AddCurrencyResponse{}); })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::INTERNAL, ex.what()));
                })
                .fail(
                    [sender] { sender->finish(grpc::Status(grpc::INTERNAL, "Undefined error")); });
        });

    registerCall(&tradingPairs::AsyncService::RequestEnableTradingPair, tradingPairsService,
        [this](auto /*context*/, auto request, auto sender) {
            _swapService.activatePair(request->pairid())
                .then([sender] { sender->finish(EnableTradingPairResponse{}); })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::INTERNAL, ex.what()));
                })
                .fail(
                    [sender] { sender->finish(grpc::Status(grpc::INTERNAL, "Undefined error")); });
        });

    registerCall(&tradingPairs::AsyncService::RequestGetActiveTradingPair, tradingPairsService,
        [this](auto /*context*/, auto request, auto sender) {
            _swapService.activatedPairs().then([sender](std::vector<std::string> activatedPairs) {
                GetActiveTradingPairResponse response;

                for (auto&& order : activatedPairs) {
                    *response.add_pairid() = order;
                }

                sender->finish(response);
            });
        });

    registerCall(&currencies::AsyncService::RequestGetAddedCurrencies, currenciesService,
        [this](auto /*context*/, auto request, auto sender) {
            _swapService.listCurrencies().then([sender](std::vector<std::string> listCurrencies) {
                GetAddedCurrenciesResponse response;

                for (auto&& currency : listCurrencies) {
                    *response.add_currency() = currency;
                }

                sender->finish(response);
            });
        });

    registerCall(&orders::AsyncService::RequestCancelOrder, ordersService,
        [this](auto /*context*/, auto request, auto sender) {
            _swapService.cancelOrder(request->pairid(), request->orderid())
                .then([sender] { sender->finish(CancelOrderResponse{}); })
                .fail([sender](const std::exception& error) {
                    CancelOrderResponse resp;
                    grpc::Status status(grpc::StatusCode::ABORTED, std::string(error.what()));
                    sender->finish(resp, status);
                });
        });

    registerCall(&orders::AsyncService::RequestListOrders, ordersService,
        [this](auto /*context*/, auto request, auto sender) {
            auto listOrders = _swapService.listOrders(
                request->pairid(), request->lastknownprice(), request->limit());

            listOrders
                .then([sender](std::vector<orderbook::LimitOrder> orders) {
                    ListOrdersResponse response;
                    for (auto&& order : orders) {
                        auto& r = *response.add_orders();
                        r.set_pairid(order.pairId);
                        r.set_allocated_price(ConvertToBigInt(order.price).release());
                        r.set_allocated_funds(ConvertToBigInt(order.quantity).release());
                    }

                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::INTERNAL, ex.what()));
                })
                .fail(
                    [sender] { sender->finish(grpc::Status(grpc::INTERNAL, "Undefined error")); });
        });

    registerCall(&orders::AsyncService::RequestListOwnOrders, ordersService,
        [this](auto /*context*/, auto request, auto sender) {
            auto listOrders = _swapService.listOwnOrders(request->pairid());

            listOrders
                .then([sender](std::vector<orderbook::OwnOrder> orders) {
                    ListOwnOrdersResponse response;
                    for (auto&& order : orders) {
                        FillOrderHelper(*response.add_orders(), order);
                    }

                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::INTERNAL, ex.what()));
                })
                .fail(
                    [sender] { sender->finish(grpc::Status(grpc::INTERNAL, "Undefined error")); });
        });

#if 0
    registerCall(&LSSD::AsyncService::RequestDeactivatePair, coreService, [this](auto /*context*/, auto request, auto sender) {
        _swapService.deactivatePair(request->pair_id()).then([sender] {
            sender->finish(ActivatePairResponse{});
        });
    });

    registerCall(&LSSD::AsyncService::RequestListTrades, coreService, [this](auto /*context*/, auto request, auto sender) {

    });

    registerCall(&LSSD::AsyncService::RequestExecuteSwap, coreService, [](auto /*context*/, auto request, auto sender) {

    });

    registerCall(&LSSD::AsyncService::RequestListCurrencies, coreService, [this](auto /*context*/, auto request, auto sender) {
        _swapService.listCurrencies().then([sender] {
            sender->finish(ListCurrenciesResponse{});
        });
    });

#endif

    registerCall(&orders::AsyncService::RequestPlaceOrder, ordersService,
        [this](auto /*context*/, auto request, auto sender) {
            try {
                orderbook::LimitOrder order(
                    orderbook::MarketOrder{ ConvertBigInt(request->funds()), request->pairid(),
                        request->side() == lssdrpc::OrderSide::buy },
                    ConvertBigInt(request->price()));

                std::replace(order.pairId.begin(), order.pairId.end(), '/', '_');

                _swapService.placeOrder(order)
                    .then([sender](::swaps::PlaceOrderResult outcome) {
                        PlaceOrderResponse resp;
                        if (auto ownOrder = boost::get<orderbook::OwnOrder>(&outcome)) {
                            auto* order = new lssdrpc::Order;
                            order->set_orderid(ownOrder->id);
                            order->set_side(ownOrder->isBuy ? OrderSide::buy : OrderSide::sell);
                            order->set_allocated_price(ConvertToBigInt(ownOrder->price).release());
                            order->set_pairid(ownOrder->pairId);
                            order->set_isownorder(true);
                            order->set_allocated_funds(
                                ConvertToBigInt(ownOrder->quantity).release());
                            resp.set_allocated_order(order);
                        } else if (auto swapInfo = boost::get<::swaps::SwapSuccess>(&outcome)) {
                            auto success = new lssdrpc::SwapSuccess;
                            success->set_role(swapInfo->role == ::swaps::SwapRole::Taker
                                    ? SwapSuccess_Role::SwapSuccess_Role_TAKER
                                    : SwapSuccess_Role::SwapSuccess_Role_MAKER);
                            success->set_allocated_price(
                                ConvertToBigInt(swapInfo->price).release());
                            success->set_rhash(swapInfo->rHash);
                            success->set_rpreimage(swapInfo->rPreimage);
                            success->set_pairid(swapInfo->pairId);
                            success->set_orderid(swapInfo->orderId);
                            success->set_allocated_funds(
                                ConvertToBigInt(swapInfo->quantity).release());
                            success->set_allocated_amountsent(
                                ConvertToBigInt(swapInfo->amountSent).release());
                            success->set_allocated_amountreceived(
                                ConvertToBigInt(swapInfo->amountReceived).release());
                            success->set_currencysent(swapInfo->currencySent);
                            success->set_currencyreceived(swapInfo->currencyReceived);
                            resp.set_allocated_swapsuccess(success);
                        } else {
                            auto undefinedFailure = new lssdrpc::SwapFailure;
                            undefinedFailure->set_failurereason("Undefined swap outcome");
                            auto failure = new lssdrpc::PlaceOrderFailure;
                            failure->set_allocated_swapfailure(undefinedFailure);
                            resp.set_allocated_failure(failure);
                        }

                        sender->finish(resp);
                    })
                    .fail([sender](::swaps::PlaceOrderFailure placeOrderFailure) {
                        PlaceOrderResponse resp;
                        auto failure = new lssdrpc::PlaceOrderFailure;
                        resp.set_allocated_failure(failure);

                        if (auto swapFailure
                            = boost::get<::swaps::SwapFailure>(&placeOrderFailure)) {
                            auto swapFailureOut = new lssdrpc::SwapFailure;
                            swapFailureOut->set_pairid(swapFailure->pairId);
                            swapFailureOut->set_orderid(swapFailure->orderId);
                            swapFailureOut->set_allocated_funds(
                                ConvertToBigInt(swapFailure->quantity).release());
                            swapFailureOut->set_failurereason(swapFailure->failureMessage);
                            failure->set_allocated_swapfailure(swapFailureOut);
                        } else if (auto feeFailure
                            = boost::get<::swaps::FeeFailure>(&placeOrderFailure)) {
                        }

                        sender->finish(resp);
                    })
                    .fail([sender](const std::exception& error) {
                        PlaceOrderResponse resp;
                        resp.mutable_failure()->mutable_swapfailure()->set_failurereason(
                            error.what());
                        sender->finish(resp);
                    })
                    .fail([sender] {
                        PlaceOrderResponse resp;
                        resp.mutable_failure()->mutable_swapfailure()->set_failurereason(
                            "Undefined swap outcome");
                        sender->finish(resp);
                    });
            } catch (std::exception& ex) {
                PlaceOrderResponse resp;
                resp.mutable_failure()->mutable_swapfailure()->set_failurereason(ex.what());
                sender->finish(resp);
                return;
            }
        });

#if 0
    registerCall(&LSSD::AsyncService::RequestRemoveCurrency, coreService, [](auto /*context*/, auto request, auto sender) {

    });

#endif

    registerCall(&orders::AsyncService::RequestSubscribeOrders, ordersService,
        [this](auto /*context*/, auto /*request*/, auto stream) {
            _swapsServiceNotifications.addSubscription(stream);
        });

    registerCall(&orders::AsyncService::RequestSubscribeOwnOrders, ordersService,
        [this](auto /*context*/, auto /*request*/, auto stream) {
            _swapsServiceNotifications.addSubscription(stream);
        });

    registerCall(&lssdrpc::swaps::AsyncService::RequestSubscribeSwaps, swapsService,
        [this](auto /*context*/, auto /*request*/, auto stream) {
            _swapsServiceNotifications.addSubscription(stream);
        });

    registerCall(&lssdrpc::orders::AsyncService::RequestSubscribeOrderbookState, ordersService,
        [this](auto /*context*/, auto /*request*/, auto stream) {
            _swapsServiceNotifications.addSubscription(stream);
        });

    registerCall(&lssdrpc::renting::AsyncService::RequestGetFeeToRentChannel, rentingService,
        [this](auto /*context*/, auto request, auto sender) {
            _swapService.orderBookClient()
                ->renting()
                ->feeToRentChannel(request->currency(), request->payingcurrency(),
                    ConvertBigInt(request->capacity()), request->lifetimeseconds())
                .then([sender](orderbook::OrderbookChannelRenting::RentingFeeResponse response) {
                    GetFeeToRentChannelResponse r;
                    r.mutable_fee()->set_value(response.fee().value());
                    r.mutable_rentingfee()->set_value(response.rentingfee().value());
                    r.mutable_onchainfees()->set_value(response.onchainfees().value());
                    sender->finish(r);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::INTERNAL, ex.what()));
                });
        });

    registerCall(&lssdrpc::renting::AsyncService::RequestGenerateRentChannelInvoice, rentingService,
        [this](auto /*context*/, auto request, auto sender) {
            _swapService.orderBookClient()
                ->renting()
                ->generateRentInvoice(request->currency(), request->payingcurrency(),
                    ConvertBigInt(request->capacity()), request->lifetimeseconds())
                .then([sender](orderbook::OrderbookChannelRenting::RentInvoice response) {
                    GenerateInvoiceToRentChannelResponse r;
                    r.set_paymentrequest(response);
                    sender->finish(r);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::INTERNAL, ex.what()));
                });
        });

    registerCall(&lssdrpc::renting::AsyncService::RequestRentChannel, rentingService,
        [this](auto /*context*/, auto request, auto sender) {
            _swapService.orderBookClient()
                ->renting()
                ->rent(request->paymenthash(), request->payingcurrency())
                .then([sender](orderbook::OrderbookChannelRenting::RentResponse response) {
                    lssdrpc::RentChannelResponse r;
                    r.set_channelid(response.channelid());
                    sender->finish(r);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::INTERNAL, ex.what()));
                });
        });
}

//==============================================================================
