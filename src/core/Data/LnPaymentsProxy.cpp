#include "LnPaymentsProxy.hpp"

#include <Chain/AbstractTransactionsCache.hpp>
#include <LndTools/LndGrpcClient.hpp>
#include <Tools/LndTools.hpp>
#include <Utils/Logging.hpp>

#include <LndTools/LndTypes.hpp>
#include <LndTools/Protos/invoices.grpc.pb.h>
#include <LndTools/Protos/router.grpc.pb.h>

using namespace lnrpc;

//==============================================================================

using OnPaymentsReady = std::function<void(const std::vector<lnrpc::Payment>&)>;
using OnInvoicesReady = std::function<void(const std::vector<lnrpc::Invoice>&)>;

//==============================================================================

static void ListInvoicesHelper(
    OnInvoicesReady onInvoicesReady, uint64_t offset, QPointer<LndGrpcClient> client)
{
    if (!client) {
        return;
    }

    ListInvoiceRequest req;
    req.set_reversed(false);
    req.set_pending_only(false);
    req.set_index_offset(offset);
    req.set_num_max_invoices(100);

    client->makeRpcUnaryRequest<ListInvoiceResponse>(
              &Lightning::Stub::PrepareAsyncListInvoices, req)
        .then([=](ListInvoiceResponse response) mutable {
            if (response.invoices_size() == 0) {
                return;
            }

            std::vector<lnrpc::Invoice> invoices;
            invoices.reserve(response.invoices_size());
            for (auto&& payment : response.invoices()) {
                invoices.emplace_back(payment);
            }

            onInvoicesReady(invoices);

            ListInvoicesHelper(onInvoicesReady, response.last_index_offset(), client);
        });
}

//==============================================================================

static void ListPaymentsHelper(
    OnPaymentsReady onPaymentsReady, uint64_t offset, QPointer<LndGrpcClient> client)
{
    if (!client) {
        return;
    }

    ListPaymentsRequest req;
    req.set_reversed(false);
    req.set_include_incomplete(false);
    req.set_index_offset(offset);
    req.set_max_payments(100);

    client
        ->makeRpcUnaryRequest<ListPaymentsResponse>(&Lightning::Stub::PrepareAsyncListPayments, req)
        .then([=](ListPaymentsResponse response) mutable {
            if (response.payments_size() == 0) {
                return;
            }

            std::vector<lnrpc::Payment> payments;
            payments.reserve(response.payments_size());
            for (auto&& payment : response.payments()) {
                payments.emplace_back(payment);
            }

            onPaymentsReady(payments);

            ListPaymentsHelper(onPaymentsReady, response.last_index_offset(), client);
        });
}

//==============================================================================

LnPaymentsProxy::LnPaymentsProxy(
    AssetID assetID, AbstractTransactionsCache* txCache, LndGrpcClient* client, QObject* parent)
    : QObject(parent)
    , _client(client)
    , _txCache(txCache)
    , _assetID(assetID)
{
    init();
}

//==============================================================================

Promise<QString> LnPaymentsProxy::addInvoice(int64_t satoshisAmount)
{
    return Promise<QString>([this, satoshisAmount](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            Invoice req;
            req.set_amt_paid_sat(satoshisAmount);
            req.set_value(satoshisAmount);
            req.set_private_(true);

            _client
                ->makeRpcUnaryRequest<AddInvoiceResponse>(
                    &Lightning::Stub::PrepareAsyncAddInvoice, req)
                .then([resolve, reject, this](AddInvoiceResponse response) {
                    this->subscribeSingleInvoice(
                        response.r_hash(), lndtypes::LightingInvoiceReason::INVOICE_PAYMENT);
                    resolve(QString::fromStdString(response.payment_request()));
                })
                .fail([reject](Status status) {
                    LogCDebug(Lnd)
                        << "Failed to execute addInvoice" << status.error_message().c_str();
                    reject(QString::fromStdString(status.error_message()));
                    throw;
                });
        });
    });
}

//==============================================================================

Promise<QString> LnPaymentsProxy::makePayment(QString paymentRequest)
{
    return Promise<QString>([this, paymentRequest](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            routerrpc::SendPaymentRequest req;
            req.set_no_inflight_updates(true);
            req.set_payment_request(paymentRequest.toStdString());
            req.set_timeout_seconds(60);
            req.set_fee_limit_sat(100000000);
            req.set_max_parts(100);

            auto readContext = ObserveAsync<Payment>(this,
                [assetId = _assetID, this, resolve, reject](auto response) {

                auto parsed = ParseLnPayment(assetId, response);
                parsed->tx().set_type(lndtypes::LightningPaymentReason::PAYMENT_PAYMENT);
                _txCache->addTransactionsSync({ parsed });

                if (response.failure_reason() == PaymentFailureReason::FAILURE_REASON_NONE) {
                    resolve(QString::fromStdString(response.payment_hash()));
                    } else {
                        LogCCritical(Lnd)
                            << "Failed to execute makePayment with raw payment request"
                            << response.failure_reason();
                        reject(std::runtime_error(PaymentFailureMsg(response.failure_reason())));
                    }
                },
                [reject](
                    grpc::Status status) { reject(std::runtime_error(status.error_message())); });

            _client->makeRouterStreamingRequest(&routerrpc::Router::Stub::PrepareAsyncSendPaymentV2,
                req, std::move(readContext), 0);
        });
    });
}

//==============================================================================

void LnPaymentsProxy::onConnected()
{
    if (!_client->isConnected()) {
        return;
    }

    if (_hasConnection) {
        return;
    }

    using lnrpc::Invoice;

    lnrpc::InvoiceSubscription invoiceSub;

    auto context = ObserveAsync<lnrpc::Invoice>(this,
        [this](auto invoice) {
            if (invoice.state() == lnrpc::Invoice_InvoiceState::Invoice_InvoiceState_OPEN) {
                this->subscribeSingleInvoice(
                    invoice.r_hash(), lndtypes::LightingInvoiceReason::UNKNOWN_INVOICE_REASON);
            }
        },
        [this](auto status) {
            _hasConnection = false;
            _connectionTimer->start(_connectionTimer->interval());
        });

    _client->makeRpcStreamingRequest(
        &lnrpc::Lightning::Stub::PrepareAsyncSubscribeInvoices, invoiceSub, std::move(context), 0);

    _hasConnection = true;

    const auto& payments = _txCache->lnPaymentsListSync();
    const auto& invoices = _txCache->lnInvoicesListSync();
    auto paymentsOffset = payments.empty() ? 0 : payments.back()->paymentIndex() + 1;
    auto invoicesOffset = invoices.empty() ? 0 : invoices.back()->addIndex() + 1;

    for (auto&& invoice : invoices) {
        if (invoice->state()
            == chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_OPEN) {
            subscribeSingleInvoice(invoice->tx().r_hash(), invoice->type());
        }
    }

    auto txCache = _txCache;
    auto onDataReady = [txCache, assetID = _assetID](const auto& from, auto transform) {
        if (!txCache) {
            return;
        }

        std::vector<::Transaction> txns(from.size());
        std::transform(std::begin(from), std::end(from), std::begin(txns),
            [assetID, transform](
                const auto& lnPayment) { return ::Transaction{ transform(assetID, lnPayment) }; });

        txCache->addTransactionsSync(txns);
    };

    ListPaymentsHelper(
        std::bind(onDataReady, std::placeholders::_1, ParseLnPayment), paymentsOffset, _client);
    ListInvoicesHelper(
        std::bind(onDataReady, std::placeholders::_1, ParseLnInvoice), invoicesOffset, _client);
}

//==============================================================================

void LnPaymentsProxy::init()
{
    connect(_client, &LndGrpcClient::connected, this, &LnPaymentsProxy::onConnected);

    _connectionTimer = new QTimer(this);
    _connectionTimer->setInterval(5000);
    connect(_connectionTimer, &QTimer::timeout, this, &LnPaymentsProxy::onConnected);
    _connectionTimer->start(_connectionTimer->interval());

    onConnected();
}

//==============================================================================

void LnPaymentsProxy::subscribeSingleInvoice(
    std::string rHash, lndtypes::LightingInvoiceReason initialReason)
{
    invoicesrpc::SubscribeSingleInvoiceRequest req;
    req.set_r_hash(rHash);
    auto context = ObserveAsync<Invoice>(this,
        [this, initialReason](auto invoice) {
            std::vector<::Transaction> txns;

            const auto& invoices = _txCache->lnInvoicesListSync();
            auto newInvoice = ParseLnInvoice(_assetID, invoice);
            if (invoice.state() != Invoice::InvoiceState::Invoice_InvoiceState_OPEN) {
                // means that we have old invoice, let's carefuly merge it in case there are
                // some sensitive data like type and memo
                if (auto oldInvoice
                    = TransactionUtils::FindInvoice(invoices, invoice.add_index())) {
                    newInvoice->tx().set_type(oldInvoice->type());
                    *newInvoice->tx().mutable_memo() = oldInvoice->tx().memo();
                }
                txns.emplace_back(newInvoice);
            } else {
                if (!TransactionUtils::FindInvoice(invoices, invoice.add_index())) {
                    newInvoice->tx().set_type(initialReason);
                    txns.emplace_back(newInvoice);
                }
            }

            if (!txns.empty()) {
                _txCache->addTransactionsSync(txns);
            }
        },
        [](auto) {

        });
    _client->makeInvoicesStreamingRequest(
        &invoicesrpc::Invoices::Stub::PrepareAsyncSubscribeSingleInvoice, req, std::move(context),
        0);
}

//==============================================================================
