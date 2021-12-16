#include "LndSwapClient.hpp"
#include <Utils/Logging.hpp>
#include <Utils/Utils.hpp>
#include <algorithm>
#include <cmath>
#include <utilstrencodings.h>

#include <LndTools/LndGrpcClient.hpp>

using namespace lnrpc;
using namespace invoicesrpc;
using namespace routerrpc;

//==============================================================================

namespace swaps {

//==============================================================================

LndSwapClient::LndSwapClient(LndGrpcClient* grpcClient, std::string currency, QObject* parent)
    : AbstractSwapLndClient(parent)
    , _grpcClient(grpcClient)
    , _executionContext(new QObject(this))
    , _currency(currency)
{
}

//==============================================================================

Promise<void> LndSwapClient::verifyConnection()
{
    return getHeight().then([] {});
}

//==============================================================================

static std::string FromHex(std::string hexHash)
{
    auto vec = bitcoin::ParseHex(hexHash);
    return std::string(std::begin(vec), std::end(vec));
}

//==============================================================================

Promise<std::string> LndSwapClient::sendPayment(SwapDeal deal)
{
    return sendSwapPayment(deal)
        .then([](lnrpc::Payment payment) { return payment.payment_preimage(); })
        .tapFail([](PaymentFailureReason reason) {
            throw std::runtime_error(PaymentFailureMsg(reason));
        });
}

//==============================================================================

Promise<Payment> LndSwapClient::sendPayment(
    std::string paymentRequest, lndtypes::LightningPaymentReason reason)
{
    Q_UNUSED(reason)
    return Promise<Payment>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            SendPaymentRequest req;
            req.set_no_inflight_updates(true);
            req.set_payment_request(paymentRequest);
            req.set_timeout_seconds(60);
            req.set_fee_limit_sat(100000000);
            req.set_max_parts(100);

            auto readContext = ObserveAsync<Payment>(this,
                [this, resolve, reject, paymentRequest](auto response) {
                    if (response.failure_reason() == PaymentFailureReason::FAILURE_REASON_NONE) {
                        resolve(response);
                    } else {
                        LogCCritical(Lnd)
                            << "Failed to execute makePayment with raw payment request:"
                            << response.failure_reason() << paymentRequest.data();
                        reject(response);
                    }
                },
                [this, reject, paymentRequest](auto status) {
                    if (!status.ok()) {
                        LogCCritical(Lnd)
                            << "Failed to execute makePayment with raw payment request:"
                            << status.error_code() << status.error_message().data()
                            << paymentRequest.data();
                        reject(std::runtime_error(status.error_message()));
                    }
                });

            _grpcClient->makeRouterStreamingRequest(
                &Router::Stub::PrepareAsyncSendPaymentV2, req, std::move(readContext), 0);
        });
    });
}

//==============================================================================

Promise<std::string> LndSwapClient::addHodlInvoice(std::string rHash, u256 units, uint32_t cltvExpiry)
{
    return Promise<std::string>([this, rHash, units, cltvExpiry](
                                    const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            AddHoldInvoiceRequest req;

            auto rHashRaw = FromHex(rHash);
            LogCDebug(Lnd) << "addInvoice"
                           << "rHash" << QString::fromStdString(rHash) << "rHashRaw"
                           << QString::fromStdString(rHashRaw);
            req.set_hash(rHashRaw);
            req.set_value(units.convert_to<int64_t>());
            req.set_cltv_expiry(cltvExpiry);
            req.set_private_(true);

            _grpcClient
                ->makeInvoicesUnaryRequest<AddHoldInvoiceResp>(
                    &Invoices::Stub::PrepareAsyncAddHoldInvoice, req)
                .then([this, rHashRaw, resolve, reject](AddHoldInvoiceResp response) {
                    this->subscribeSingleInvoice(rHashRaw);
                    resolve(response.payment_request());
                })
                .fail([reject](grpc::Status status) { reject(status.error_message().c_str()); });
        });
    });
}

//==============================================================================

Promise<AddInvoiceResponse> LndSwapClient::addInvoice(
    int64_t units, lndtypes::LightingInvoiceReason reason)
{
    Q_UNUSED(reason)
    return Promise<AddInvoiceResponse>([this, units](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            Invoice req;
            req.set_value(units);
            req.set_private_(true);

            _grpcClient
                ->makeRpcUnaryRequest<AddInvoiceResponse>(
                    &Lightning::Stub::PrepareAsyncAddInvoice, req)
                .then([resolve, reject](AddInvoiceResponse response) { resolve(response); })
                .fail([reject](
                          Status status) { reject(std::runtime_error(status.error_message())); });
        });
    });
}

//==============================================================================

Promise<void> LndSwapClient::settleInvoice(std::string /*rHash*/, std::string rPreimage, QVariantMap payload)
{
    return Promise<void>([this, rPreimage](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            SettleInvoiceMsg req;
            req.set_preimage(FromHex(rPreimage));

            _grpcClient
                ->makeInvoicesUnaryRequest<SettleInvoiceResp>(
                    &Invoices::Stub::PrepareAsyncSettleInvoice, req)
                .then([resolve, reject](SettleInvoiceResp /*response*/) { resolve(); })
                .fail([reject](grpc::Status status) {
                    LogCCritical(Lnd)
                        << "Failed to execute settleInvoice" << status.error_message().c_str();
                    reject();
                });
        });
    });
}

//==============================================================================

Promise<void> LndSwapClient::removeInvoice(std::string rHash)
{
    return Promise<void>([this, rHash](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            if (!_grpcClient->isConnected()) {
                reject();
                return;
            }

            CancelInvoiceMsg req;
            req.set_payment_hash(FromHex(rHash));

            LogCDebug(Lnd) << "removeInvoice"
                           << "rHash" << QString::fromStdString(rHash);

            _grpcClient
                ->makeInvoicesUnaryRequest<CancelInvoiceResp>(
                    &Invoices::Stub::PrepareAsyncCancelInvoice, req)
                .then([resolve, reject](CancelInvoiceResp) { resolve(); })
                .fail([reject](grpc::Status status) {
                    LogCCritical(Lnd)
                        << "Failed to execute removeInvoice" << status.error_message().c_str();
                    reject();
                });
        });
    });
}

//==============================================================================

Promise<AbstractSwapClient::Routes> LndSwapClient::getRoutes(
    u256 units, std::string destination, std::string currency, uint32_t finalCltvDelta)
{
    return Promise<AbstractSwapClient::Routes>([this, units, destination, currency, finalCltvDelta](
                                                   const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            QueryRoutesRequest req;
            req.set_amt(units.convert_to<int64_t>());
            req.set_pub_key(destination);
            req.set_final_cltv_delta(finalCltvDelta);

            LogCDebug(Swaps) << "Lnd client, query routes with next params:"
                             << "amt:" << units.convert_to<int64_t>() << "pub_key:" << destination.c_str()
                             << "final_cltv_delta" << finalCltvDelta;

            _grpcClient
                ->makeRpcUnaryRequest<QueryRoutesResponse>(
                    &Lightning::Stub::PrepareAsyncQueryRoutes, req)
                .then([resolve, reject](QueryRoutesResponse response) {
                    Routes routes;

                    for (auto&& route : response.routes()) {
                        Route rt;
                        rt.total_time_lock = route.total_time_lock();
                        rt.total_fees_msat = route.total_amt_msat();
                        rt.total_amt_msat = route.total_amt_msat();

                        std::vector<Route::Hop> hops;

                        for (auto&& hop : route.hops()) {
                            Route::Hop hp;
                            hp.chan_id = hop.chan_id();
                            hp.chan_capacity = hop.chan_capacity();
                            hp.fee = hop.fee();
                            hp.expiry = hop.expiry();
                            hp.amt_to_forward_msat = hop.amt_to_forward_msat();
                            hp.fee_msat = hop.fee_msat();
                            hp.pub_key = hop.pub_key();

                            hops.emplace_back(hp);
                        }

                        routes.emplace_back(rt);
                    }

                    resolve(routes);
                })
                .fail([reject](grpc::Status status) {
                    LogCCritical(Lnd)
                        << "Failed to execute getRoutes" << status.error_message().c_str();
                    reject(std::runtime_error((status.error_message())));
                });
        });
    });
}

//==============================================================================

Promise<uint32_t> LndSwapClient::getHeight()
{
    return Promise<uint32_t>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            _grpcClient
                ->makeRpcUnaryRequest<GetInfoResponse>(
                    &Lightning::Stub::PrepareAsyncGetInfo, GetInfoRequest())
                .then([resolve](GetInfoResponse response) {
                    uint32_t blockHeight = response.block_height();
                    resolve(blockHeight);
                })
                .fail([reject](grpc::Status status) {
                    LogCCritical(Lnd)
                        << "Failed to execute getinfo" << status.error_message().c_str();
                    reject();
                });
        });
    });
}

//==============================================================================

Promise<LightningPayRequest> LndSwapClient::decodePayRequest(std::string paymentRequest)
{
    return Promise<LightningPayRequest>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            PayReqString payReq;
            payReq.set_pay_req(paymentRequest);

            _grpcClient
                ->makeRpcUnaryRequest<PayReq>(&Lightning::Stub::PrepareAsyncDecodePayReq, payReq)
                .then([resolve, this](PayReq response) {
                    LightningPayRequest request;
                    request.destination = QString::fromStdString(response.destination());
                    request.paymentHash = QString::fromStdString(response.payment_hash());
                    request.numSatoshis = response.num_satoshis();

                    resolve(request);
                })
                .fail([reject](Status status) {
                    LogCDebug(Lnd)
                        << "Failed to execute decodePayRequest" << status.error_message().c_str();
                    reject(QString::fromStdString(status.error_message()));
                });
        });
    });
}

//==============================================================================

Promise<std::vector<LndChannel>> LndSwapClient::activeChannels() const
{
    return Promise<std::vector<LndChannel>>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            ListChannelsRequest req;
            req.set_active_only(true);
            _grpcClient
                ->makeRpcUnaryRequest<ListChannelsResponse>(
                    &Lightning::Stub::PrepareAsyncListChannels, req)
                .then([resolve](ListChannelsResponse response) {
                    std::vector<LndChannel> activeChannels;
                    for (auto&& channel : response.channels()) {
                        if (channel.active()) {
                            LndChannel activeChannel
                                = LndChannel::FromRpcChannelLndChannel(channel);
                            activeChannel.details["local_chan_reserve_sat"]
                                = QVariant::fromValue(channel.local_chan_reserve_sat());
                            activeChannel.details["initiator"]
                                = QVariant::fromValue(channel.initiator());
                            activeChannel.details["fee_per_kw"]
                                = QVariant::fromValue(channel.fee_per_kw());
                            activeChannel.details["commit_weight"]
                                = QVariant::fromValue(channel.commit_weight());
                            activeChannels.emplace_back(activeChannel);
                        }
                    }

                    resolve(activeChannels);
                })
                .fail([reject](grpc::Status status) {
                    LogCCritical(Lnd)
                        << "Failed to execute listChannels" << status.error_message().c_str();
                    reject(std::runtime_error((status.error_message())));
                });
        });
    });
}

//==============================================================================

uint32_t LndSwapClient::finalLock() const
{
    return static_cast<uint32_t>(std::round(400 / minutesPerBlock()));
}

//==============================================================================

ClientType LndSwapClient::type() const
{
    return ClientType::Lnd;
}

//==============================================================================

double LndSwapClient::minutesPerBlock() const
{
    return MINUTES_PER_BLOCK_BY_CURRENCY.at(_currency);
}

//==============================================================================

uint32_t LndSwapClient::lockBuffer() const
{
    return static_cast<uint32_t>(std::round(LOCK_BUFFER_HOURS * 60 / minutesPerBlock()));
}

//==============================================================================

bool LndSwapClient::isConnected() const
{
    return _grpcClient->isConnected();
}

//==============================================================================

Promise<std::string> LndSwapClient::destination() const
{
    return Promise<std::string>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            _grpcClient
                ->makeRpcUnaryRequest<GetInfoResponse>(
                    &Lightning::Stub::PrepareAsyncGetInfo, GetInfoRequest())
                .then([resolve](GetInfoResponse response) { resolve(response.identity_pubkey()); })
                .fail([reject](grpc::Status status) {
                    LogCCritical(Lnd)
                        << "Failed to execute getinfo" << status.error_message().c_str();
                    reject();
                });
        });
    });
}

//==============================================================================

Promise<Payment> LndSwapClient::sendSwapPayment(SwapDeal deal)
{
    return Promise<Payment>([this, deal](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            SendPaymentRequest req;
            req.set_timeout_seconds(60);
            req.set_no_inflight_updates(true);
            req.set_fee_limit_sat(100000000);
            req.set_max_parts(100);

            if (deal.role == SwapRole::Taker) {
                auto parsed = bitcoin::ParseHex(deal.destination->c_str());
                //                req.set_payment_hash(FromHex(deal.rHash));
                //                req.set_amt(deal.makerAmount);
                req.set_payment_request(deal.paymentRequest.get());
                //                req.set_dest(std::string(parsed.begin(), parsed.end()));
                //                req.set_final_cltv_delta(deal.makerCltvDelta.value());
            } else {
                auto parsed = bitcoin::ParseHex(deal.takerPubKey->c_str());
                //                req.set_payment_hash(FromHex(deal.rHash));
                //                req.set_amt(deal.takerAmount);
                req.set_payment_request(deal.paymentRequest.get());
                //                req.set_dest(std::string(parsed.begin(), parsed.end()));
                //                req.set_final_cltv_delta(deal.takerCltvDelta);
                //                req.set_cltv_limit(deal.takerMaxTimeLock.value() + 3); // TODO:
                //                investigate why we need to add 3 blocks - if not lnd says route
                //                not found
            }

            auto readContext = ObserveAsync<Payment>(this,
                [resolve, reject, req, currency = _currency](auto response) {
                    if (response.failure_reason() == PaymentFailureReason::FAILURE_REASON_NONE) {
                        resolve(response);
                    } else {
                        LogCCritical(Swaps)
                            << "Failed Sending swap payment - Payment request:"
                            << QString::fromLatin1(
                                   QByteArray::fromStdString(req.payment_request()).toHex())
                            << response.failure_reason();

                        reject(response);
                    }
                },
                [reject, req, currency = _currency](auto status) {
                    if (!status.ok()) {
                        LogCCritical(Swaps)
                            << "Failed Sending swap payment - Payment request:"
                            << QString::fromLatin1(
                                   QByteArray::fromStdString(req.payment_request()).toHex())
                            << status.error_message().data();
                        reject(std::runtime_error(status.error_message()));
                    }
                });

            _grpcClient->makeRouterStreamingRequest(
                &Router::Stub::PrepareAsyncSendPaymentV2, req, std::move(readContext), 0);
        });
    });
}

//==============================================================================

Promise<Invoice> LndSwapClient::lookupInvoice(std::string rHash)
{
    return Promise<Invoice>([=](const auto& resolve, const auto& reject) mutable {
        QMetaObject::invokeMethod(_executionContext, [=]() mutable {
            PaymentHash req;

            if (rHash.size() == 64) {
                auto parsed = bitcoin::ParseHex(rHash);
                rHash = std::string(parsed.begin(), parsed.end());
            }

            req.set_r_hash(rHash);

            _grpcClient
                ->makeRpcUnaryRequest<Invoice>(&Lightning::Stub::PrepareAsyncLookupInvoice, req)
                .then([resolve](Invoice response) { resolve(response); })
                .fail([reject](grpc::Status status) {
                    LogCCritical(Lnd)
                        << "Failed to execute lookupInvoice" << status.error_message().c_str();
                    reject();
                });
        });
    });
}

//==============================================================================

void LndSwapClient::subscribeSingleInvoice(std::string rHashRaw)
{
    SubscribeSingleInvoiceRequest req;
    req.set_r_hash(rHashRaw);
    auto context = ObserveAsync<Invoice>(this,
        [this](auto invoice) {
            if (invoice.state() == Invoice::InvoiceState::Invoice_InvoiceState_ACCEPTED) {
                LogCDebug(Lnd) << "subscribeSingleInvoice"
                               << QByteArray::fromStdString(invoice.r_hash())
                               << QByteArray::fromStdString(invoice.r_hash()).toHex();
                this->htlcAccepted(
                    QByteArray::fromStdString(invoice.r_hash()).toHex().toStdString(),
                    invoice.amt_paid_sat());
            }
        },
        [](auto status) {

        });

    _grpcClient->makeInvoicesStreamingRequest(
        &Invoices::Stub::PrepareAsyncSubscribeSingleInvoice, req, std::move(context), 0);
}

//==============================================================================
}
