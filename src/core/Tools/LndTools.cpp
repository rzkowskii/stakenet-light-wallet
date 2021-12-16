#include "LndTools.hpp"

#include <LndTools/Protos/rpc.pb.h>

using namespace lnrpc;

//==============================================================================

LightningPaymentRef ParseLnPayment(AssetID assetID, const lnrpc::Payment& payment)
{
    chain::Transaction tx;
    tx.set_asset_id(assetID);
    auto& out = *tx.mutable_lightning_payment();
    out.set_fee(payment.fee_sat());
    out.set_type(lndtypes::LightningPaymentReason::UNKNOWN_PAYMENT_REASON);
    out.set_value(payment.value_sat());
    out.set_timestamp(payment.creation_time_ns() / 1000000000);
    out.set_payment_hash(payment.payment_hash());
    out.set_payment_index(payment.payment_index());

    out.set_status([status = payment.status()]() {
        switch (status) {
        case Payment_PaymentStatus::Payment_PaymentStatus_FAILED:
            return chain::LightningPayment_PaymentStatus::LightningPayment_PaymentStatus_FAILED;
        case Payment_PaymentStatus::Payment_PaymentStatus_IN_FLIGHT:
            return chain::LightningPayment_PaymentStatus::LightningPayment_PaymentStatus_IN_FLIGHT;
        case Payment_PaymentStatus::Payment_PaymentStatus_SUCCEEDED:
            return chain::LightningPayment_PaymentStatus::LightningPayment_PaymentStatus_SUCCEEDED;
        default:
            break;
        }

        return chain::LightningPayment_PaymentStatus::LightningPayment_PaymentStatus_UNKNOWN;
    }());

    out.set_failure_reason([reason = payment.failure_reason()] {
        switch (reason) {
        case PaymentFailureReason::FAILURE_REASON_ERROR:
            return chain::PaymentFailureReason::FAILURE_REASON_ERROR;
        case PaymentFailureReason::FAILURE_REASON_TIMEOUT:
            return chain::PaymentFailureReason::FAILURE_REASON_TIMEOUT;
        case PaymentFailureReason::FAILURE_REASON_NO_ROUTE:
            return chain::PaymentFailureReason::FAILURE_REASON_NO_ROUTE;
        case PaymentFailureReason::FAILURE_REASON_INSUFFICIENT_BALANCE:
            return chain::PaymentFailureReason::FAILURE_REASON_INSUFFICIENT_BALANCE;
        case PaymentFailureReason::FAILURE_REASON_INCORRECT_PAYMENT_DETAILS:
            return chain::PaymentFailureReason::FAILURE_REASON_INCORRECT_PAYMENT_DETAILS;
        default:
            break;
        }

        return chain::PaymentFailureReason::FAILURE_REASON_NONE;
    }());

    return std::make_shared<LightningPayment>(tx);
}

//==============================================================================

LightningInvoiceRef ParseLnInvoice(AssetID assetID, const lnrpc::Invoice& invoice)
{
    chain::Transaction tx;
    tx.set_asset_id(assetID);
    auto& out = *tx.mutable_lightning_invoice();
    out.set_type(lndtypes::LightingInvoiceReason::UNKNOWN_INVOICE_REASON);
    out.set_state([state = invoice.state()] {
        switch (state) {
        case Invoice::InvoiceState::Invoice_InvoiceState_SETTLED:
            return chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_SETTLED;
        case Invoice::InvoiceState::Invoice_InvoiceState_ACCEPTED:
            return chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_ACCEPTED;
        case Invoice::InvoiceState::Invoice_InvoiceState_CANCELED:
            return chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_CANCELED;
        default:
            break;
        }

        return chain::LightningInvoice::InvoiceState::LightningInvoice_InvoiceState_OPEN;
    }());
    out.set_value(invoice.value());
    out.set_expiry(invoice.expiry());
    out.set_r_hash(invoice.r_hash());
    out.set_amt_paid(invoice.amt_paid_sat());
    out.set_add_index(invoice.add_index());
    out.set_settle_index(invoice.settle_index());
    out.set_settle_timestamp(invoice.settle_date());
    out.set_creation_timestamp(invoice.creation_date());

    return std::make_shared<LightningInvoice>(tx);
}

//==============================================================================
