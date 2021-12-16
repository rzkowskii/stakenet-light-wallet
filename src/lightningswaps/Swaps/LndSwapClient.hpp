#ifndef LNDSWAPCLIENT_HPP
#define LNDSWAPCLIENT_HPP

#include <LndTools/Protos/invoices.pb.h>
#include <LndTools/Protos/router.pb.h>

#include <LndTools/LndTypes.hpp>
#include <Swaps/AbstractSwapLndClient.hpp>

class BaseGrpcClient;
class LndGrpcClient;

namespace swaps {

//==============================================================================

class LndSwapClient : public AbstractSwapLndClient {
    Q_OBJECT
public:
    LndSwapClient(LndGrpcClient* grpcClient, std::string currency, QObject* parent = nullptr);

    Promise<void> verifyConnection() override;
    Promise<std::string> sendPayment(SwapDeal deal) override;
    Promise<lnrpc::Payment> sendPayment(
        std::string paymentRequest, lndtypes::LightningPaymentReason reason) override;
    Promise<std::string> addHodlInvoice(
        std::string rHash, u256 units, uint32_t cltvExpiry) override;
    Promise<lnrpc::AddInvoiceResponse> addInvoice(
        int64_t units, lndtypes::LightingInvoiceReason reason) override;
    Promise<void> settleInvoice(std::string rHash, std::string rPreimage, QVariantMap payload) override;
    Promise<void> removeInvoice(std::string rHash) override;
    Promise<Routes> getRoutes(u256 units, std::string destination, std::string currency,
        uint32_t finalCltvDelta) override;
    Promise<uint32_t> getHeight() override;
    Promise<LightningPayRequest> decodePayRequest(std::string paymentRequest) override;

    Promise<std::vector<LndChannel>> activeChannels() const override;

    uint32_t finalLock() const override;
    ClientType type() const override;
    double minutesPerBlock() const override;
    uint32_t lockBuffer() const override;
    bool isConnected() const override;
    Promise<std::string> destination() const override;

    Promise<lnrpc::Payment> sendSwapPayment(SwapDeal deal);
    Promise<lnrpc::Invoice> lookupInvoice(std::string rHash);

private:
    void subscribeSingleInvoice(std::string rHashRaw);

private:
    QPointer<LndGrpcClient> _grpcClient;
    QObject* _executionContext{ nullptr };
    std::string _currency;
};

//==============================================================================
}

#endif // LNDSWAPCLIENT_HPP
