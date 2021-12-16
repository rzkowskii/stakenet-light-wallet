#ifndef CONNEXTSWAPCLIENT_HPP
#define CONNEXTSWAPCLIENT_HPP

#include <Swaps/AbstractSwapConnextClient.hpp>

class AbstractConnextApi;

namespace swaps {

//==============================================================================

class ConnextResolvePaymentProxy;

//==============================================================================

class ConnextSwapClient : public AbstractSwapConnextClient {
    Q_OBJECT
public:
    explicit ConnextSwapClient(AbstractConnextApi* connextClient, std::string tokenAddress);

    // AbstractSwapClient interface
public:
    Promise<void> verifyConnection() override;
    Promise<std::string> sendPayment(SwapDeal deal) override;
    Promise<std::string> addHodlInvoice(
        std::string rHash, u256 units, uint32_t cltvExpiry) override;
    Promise<void> settleInvoice(
        std::string rHash, std::string rPreimage, QVariantMap payload) override;
    Promise<void> removeInvoice(std::string rHash) override;
    Promise<Routes> getRoutes(u256 units, std::string destination, std::string currency,
        uint32_t finalCltvDelta) override;
    Promise<std::string> destination() const override;
    Promise<uint32_t> getHeight() override;
    uint32_t finalLock() const override;
    ClientType type() const override;
    double minutesPerBlock() const override;
    uint32_t lockBuffer() const override;
    bool isConnected() const override;
    Promise<std::string> sendPaymentWithPayload(QVariantMap payload) override;
    Promise<QString> getHubChannel();
    void init(std::string token);

public slots:
    void onTransferResolved(ResolvedTransferResponse details) override;

private:
    ConnextResolvePaymentProxy* createResolveProxy(QString lockHash);
    Promise<QVector<QVariantMap>> getChannels();
    Promise<QVariantMap> tokenPayment(QVariantMap payload);

private:
    QPointer<AbstractConnextApi> _connextClient;
    mutable QString _publicIdentifier;
    QString _hubChannel;
    QString _tokenAddress;

    std::map<std::string, QPointer<ConnextResolvePaymentProxy>> _proxies;
};

//==============================================================================
}

#endif // CONNEXTSWAPCLIENT_HPP
