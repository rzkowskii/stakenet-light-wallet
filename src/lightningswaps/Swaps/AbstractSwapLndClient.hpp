#ifndef ABSTRACTSWAPLNDCLIENT_HPP
#define ABSTRACTSWAPLNDCLIENT_HPP

#include <Swaps/AbstractSwapClient.hpp>
#include <QObject>

#include <LndTools/Protos/rpc.pb.h>

namespace swaps {

class AbstractSwapLndClient : public AbstractSwapClient {
    Q_OBJECT
public:
    using AbstractSwapClient::AbstractSwapClient;

    virtual Promise<std::vector<LndChannel>> activeChannels() const = 0;
    /**
     * Sends a payment using encoded payment request
     * @returns the preimage for the payment
     */
    virtual Promise<lnrpc::Payment> sendPayment(
        std::string paymentRequest, lndtypes::LightningPaymentReason reason)
        = 0;

    /**
       @param units the amount of the invoice denominated in the smallest units supported by its
       currency
       @returns payment request
      */
    virtual Promise<lnrpc::AddInvoiceResponse> addInvoice(
        int64_t units, lndtypes::LightingInvoiceReason reason)
        = 0;

    virtual Promise<LightningPayRequest> decodePayRequest(std::string paymentRequest) = 0;

    virtual ~AbstractSwapLndClient();
};
}

#endif // ABSTRACTSWAPLNDCLIENT_HPP
