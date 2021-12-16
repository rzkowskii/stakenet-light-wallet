#ifndef PAYMENTNODEREGISTRATIONSERVICE_HPP
#define PAYMENTNODEREGISTRATIONSERVICE_HPP

#include <QObject>
#include <QPointer>
#include <Utils/Utils.hpp>
#include <unordered_map>

namespace swaps {
class AbstractSwapClientPool;
}

namespace orderbook {
class OrderbookClient;

class PaymentNodeRegistrationService : public QObject {
    Q_OBJECT
public:
    explicit PaymentNodeRegistrationService(const swaps::AbstractSwapClientPool& clients,
        orderbook::OrderbookClient* client, QObject* parent = nullptr);

    // no orderbook key registration for connext yet,
    Promise<void> ensurePaymentNodeRegistered(std::string currency);

private:
    Promise<std::string> fetchPubKey(std::string currency);
    Promise<void> registerClient(std::string currency, std::string pubkey);
    Promise<void> registerLndClient(std::string currency, std::string pubkey);
    Promise<void> registerConnextClient(std::string currency, std::string pubkey);

protected:
    const swaps::AbstractSwapClientPool& _swapClients;
    QPointer<orderbook::OrderbookClient> _orderbook;
    std::unordered_map<std::string, std::string> _registeredPubKeys;
};
}

#endif // PAYMENTNODEREGISTRATIONSERVICE_HPP
