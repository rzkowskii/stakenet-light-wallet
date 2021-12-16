#include "PaymentNodeRegistrationService.hpp"

#include <Orderbook/OrderbookClient.hpp>
#include <Swaps/AbstractSwapClient.hpp>
#include <Swaps/AbstractSwapClientPool.hpp>
#include <Swaps/Types.hpp>

#include <utilstrencodings.h>

//==============================================================================
namespace orderbook {

PaymentNodeRegistrationService::PaymentNodeRegistrationService(
    const swaps::AbstractSwapClientPool& clients, orderbook::OrderbookClient* client,
    QObject* parent)
    : QObject(parent)
    , _swapClients(clients)
    , _orderbook(client)
{
}

//==============================================================================

Promise<void> PaymentNodeRegistrationService::ensurePaymentNodeRegistered(std::string currency)
{
    auto pubkey = _registeredPubKeys[currency];
    if (pubkey.empty()) {
        QPointer<PaymentNodeRegistrationService> self{ this };
        return fetchPubKey(currency).then([self, currency](std::string pubkey) {
            if (self) {
                return self->registerClient(currency, pubkey);
            }

            return Promise<void>::resolve();
        });
    } else {
        return Promise<void>::resolve();
    }
}

//==============================================================================

Promise<std::string> PaymentNodeRegistrationService::fetchPubKey(std::string currency)
{
    if (auto client = _swapClients.getClient(currency)) {
        QPointer<PaymentNodeRegistrationService> self{ this };
        return client->destination();
    }

    return Promise<std::string>::reject(std::runtime_error("No pubkey available"));
}

//==============================================================================

Promise<void> PaymentNodeRegistrationService::registerClient(std::string currency, std::string pubkey)
{
    if(swaps::CLIENT_TYPE_PER_CURRENCY.at(currency) == swaps::ClientType::Lnd){
        return registerLndClient(currency, pubkey);
    }
    else {
        return registerConnextClient(currency, pubkey);
    }
}

//==============================================================================

Promise<void> PaymentNodeRegistrationService::registerLndClient(std::string currency, std::string pubkey)
{
    QPointer<PaymentNodeRegistrationService> self{ this };
    auto parsedPubKey = bitcoin::ParseHex(pubkey);
    return _orderbook
        ->registerLndKey(currency, std::string(parsedPubKey.begin(), parsedPubKey.end()))
        .then([self, currency, pubkey] {
            if (self) {
                self->_registeredPubKeys[currency] = pubkey;
            }
        });
}

//==============================================================================

Promise<void> PaymentNodeRegistrationService::registerConnextClient(std::string currency, std::string pubkey)
{
    QPointer<PaymentNodeRegistrationService> self{ this };
    return _orderbook->registerConnextKey(currency, pubkey).then([self, currency, pubkey] {
        if (self) {
            self->_registeredPubKeys[currency] = pubkey;
        }
    });
}

//==============================================================================

}
