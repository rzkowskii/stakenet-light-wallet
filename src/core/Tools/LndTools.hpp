#ifndef LNDTOOLS_HPP
#define LNDTOOLS_HPP

#include <Data/TransactionEntry.hpp>
#include <Tools/Common.hpp>

namespace lnrpc {
class Payment;
class Invoice;
}

LightningPaymentRef ParseLnPayment(AssetID assetID, const lnrpc::Payment& payment);
LightningInvoiceRef ParseLnInvoice(AssetID assetID, const lnrpc::Invoice& invoice);

#endif // LNDTOOLS_HPP
