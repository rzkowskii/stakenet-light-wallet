#include "ConnextSwapClient.hpp"
#include <EthCore/Types.hpp>
#include <LndTools/AbstractConnextApi.hpp>
#include <Swaps/ConnextResolvePaymentProxy.hpp>
#include <Utils/Logging.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QRandomGenerator>
#include <QVector>
#include <cmath>

namespace swaps {

static constexpr int LOCK_BUFFER_HOURS = 24;
static constexpr int PAYMENT_PROXY_TIMEOUT = 100000;

//==============================================================================
ConnextSwapClient::ConnextSwapClient(AbstractConnextApi* connextClient, std::string tokenAddress)
    : _connextClient(connextClient)
    , _tokenAddress(QString::fromStdString(tokenAddress))
{
}

//==============================================================================

Promise<void> ConnextSwapClient::verifyConnection()
{
    return destination().then([] {

    });
}

//==============================================================================

Promise<std::string> ConnextSwapClient::sendPayment(SwapDeal deal)
{
    //      "type": "HashlockTransfer",
    //      "publicIdentifier": "indra82HffkBSTeSuvuEApLqLvY4wStnQYk7ysxZfYR2MmkwEe4XBbt",
    //      "channelAddress": "0xaE55b9aDdcD1cF5F5ECbff958F8C610d58f896d9",
    //      "amount": "1234",
    //      "assetId": "0x0000000000000000000000000000000000000000",
    //      "details": {
    //        "lockHash": "0x40a089a80b03cd40d208cc383262fc3d6a8fa224ac8da55ea57998935ea73be5",
    //        "expiry": "0"
    //      },
    //      "recipient": "indra6RJffyPFY7D4pAkyooRxgXVyocxYkRUW3NFQVznHazn98vcadh",

    Q_ASSERT(deal.state == SwapState::Active);
    Q_ASSERT(deal.destination);

    u256 units;
    std::string assetId;
    decltype(deal.makerCltvDelta) expiry;

    if (deal.role == SwapRole::Maker) {
        units = deal.takerUnits;
        assetId = _tokenAddress.toStdString();
        expiry = deal.takerCltvDelta;
    } else {
        units = deal.makerUnits;
        assetId = _tokenAddress.toStdString();
        expiry = deal.makerCltvDelta;
    }
    QVariantMap payload;

    payload["amount"] = QString::fromStdString(units.str());
    payload["assetId"] = QString::fromStdString(assetId);
    payload["recipient"] = QString::fromStdString(deal.destination.get());

    getHubChannel()
        .then([&](QString channelAddr) { payload["channelAddress"] = channelAddr; })
        .wait();

    QVariantMap details;
    auto lockHash = QString("0x%1").arg(QString::fromStdString(deal.rHash));

    if (!deal.rHash.empty()) {
        details["lockHash"] = lockHash;
        details["expiry"] = 2000000000; // expiry.get();
    }

    payload["details"] = details;

    return destination().then([this, payload, rHash = deal.rHash](std::string dest) mutable {
        payload["publicIdentifier"] = QString::fromStdString(dest);
        auto proxy = createResolveProxy(QString::fromStdString(rHash));
        tokenPayment(payload);
        return QtPromise::connect(proxy, &ConnextResolvePaymentProxy::resolved)
            .then([](std::string preimage) { return preimage; })
            .timeout(PAYMENT_PROXY_TIMEOUT);
    });
}

//==============================================================================

Promise<std::string> ConnextSwapClient::sendPaymentWithPayload(QVariantMap payload)
{
    payload["assetId"] = QString::fromStdString(_tokenAddress.toStdString());
    payload["transferTimeout"] = QString::fromStdString("3600");

    getHubChannel()
        .then([&](QString channelAddr) { payload["channelAddress"] = channelAddr; })
        .wait();

    return destination().then([this, payload](std::string dest) mutable {
        payload["publicIdentifier"] = QString::fromStdString(dest);
        return tokenPayment(payload).then([](QVariantMap payload) {
            auto details = payload["details"].toJsonObject(); //.toString().toStdString();
            return details.value("lockHash").toString().toStdString();
        });
    });
}

//==============================================================================

Promise<std::string> ConnextSwapClient::addHodlInvoice(
    std::string rHash, u256 units, uint32_t cltvExpiry)
{
    Q_UNUSED(rHash)
    Q_UNUSED(units)
    Q_UNUSED(cltvExpiry)
    return QtPromise::resolve(std::string{});
}

//==============================================================================

Promise<void> ConnextSwapClient::settleInvoice(
    std::string rHash, std::string rPreimage, QVariantMap payload)
{
    Q_UNUSED(rHash)
    return destination().then([this, payload, rPreimage](std::string dest) mutable {
        payload["publicIdentifier"] = QString::fromStdString(dest);
        QVariantMap transferResolver;
        transferResolver["preImage"] = QString::fromStdString("0x" + rPreimage);
        payload["transferResolver"] = transferResolver;

        return _connextClient->transferResolve(payload).then([](QByteArray /*response*/) {});
    });
}

//==============================================================================

Promise<void> ConnextSwapClient::removeInvoice(std::string rHash)
{
    Q_UNUSED(rHash);
    return QtPromise::resolve();
}

//==============================================================================

Promise<AbstractSwapClient::Routes> ConnextSwapClient::getRoutes(
    u256 units, std::string destination, std::string currency, uint32_t finalCltvDelta)
{
    auto hubDestination = "vector6At5HhbfhcE1p1SBTxAL5ej71AGWkSSxrSTmdj6wuFHquJ1hg8";

    return getChannels().then([destination, hubDestination, units, currency, this](
                                  QVector<QVariantMap> channels) {
        LogCDebug(Swaps) << " got local channels " << channels.size();

        for (auto&& channel : channels) {
            if (channel.value("aliceIdentifier").toString().toStdString() == destination
                || channel.value("aliceIdentifier").toString().toStdString() == hubDestination) {

                eth::u256 balance{ 0 };
                auto tokenIds = channel.value("assetIds").value<QVariantList>();
                for (auto i = 0; i < tokenIds.size(); ++i) {
                    if (tokenIds[i].toString() == _tokenAddress) {

                        LogCDebug(Swaps)
                            << " Got next connext balances " << channel.value("balances");
                        balance = eth::u256{ channel.value("balances")
                                                 .value<QVariantList>()
                                                 .at(i)
                                                 .toMap()
                                                 .value("amount")
                                                 .value<QVariantList>()
                                                 .at(1)
                                                 .toString()
                                                 .toStdString() };
                    }
                }

                if (balance >= units) {
                    LogCDebug(Swaps) << "found a channel to " << QString::fromStdString(destination)
                                     << " with " << balance.str().c_str() << " balance";

                    _hubChannel = channel.value("channelAddress").toString();
                    Route route;
                    route.total_time_lock = 101;
                    return AbstractSwapClient::Routes{ route };
                }

            } else if (channel.value("bobIdentifier").toString().toStdString() == destination
                || channel.value("bobIdentifier").toString().toStdString() == hubDestination) {

                long balance{ 0 };
                auto tokenIds = channel.value("assetIds").value<QVariantList>();

                for (auto i = 0; i < tokenIds.size(); ++i) {
                    if (tokenIds[i].toString() == _tokenAddress) {

                        LogCDebug(Swaps)
                            << " Got next connext balances " << channel.value("balances");
                        balance = channel.value("balances")
                                      .value<QVariantList>()
                                      .at(i)
                                      .toMap()
                                      .value("amount")
                                      .value<QVariantList>()
                                      .first()
                                      .toString()
                                      .toLong();
                    }
                }

                if (balance >= units) {
                    LogCDebug(Swaps)
                        << "found a direct channel to " << QString::fromStdString(destination)
                        << " with " << balance << " balance";

                    _hubChannel = channel.value("channelAddress").toString();
                    Route route;
                    route.total_time_lock = 101;
                    return AbstractSwapClient::Routes{ route };
                }
            }
        }

        LogCDebug(Swaps) << "found no channel to " << QString::fromStdString(destination)
                         << " with " << currency.c_str() << " balance";
        return AbstractSwapClient::Routes{};
    });
}

//==============================================================================

Promise<std::string> ConnextSwapClient::destination() const
{
    auto h = [this] {
        if (_publicIdentifier.isEmpty()) {
            return _connextClient->getPublicIdentifier().then([this](QString identifier) {
                if (identifier != _publicIdentifier) {
                    _publicIdentifier = identifier;
                }
            });
        } else {
            return QtPromise::resolve();
        }
    };
    return h().then([this] { return _publicIdentifier.toStdString(); });
}

//==============================================================================

Promise<uint32_t> ConnextSwapClient::getHeight()
{
    return QtPromise::resolve(1u);
}

//==============================================================================

void ConnextSwapClient::init(std::string token)
{
    _tokenAddress = QString::fromStdString(token);
}

//==============================================================================

ConnextResolvePaymentProxy* ConnextSwapClient::createResolveProxy(QString lockHash)
{
    auto lockHashStd = lockHash.toStdString();
    auto proxy = new ConnextResolvePaymentProxy(lockHashStd, this);
    _proxies[lockHashStd] = proxy;
    return proxy;
}

//==============================================================================

Promise<QVariantMap> ConnextSwapClient::tokenPayment(QVariantMap payload)
{
    //      "type": "HashlockTransfer", +
    //      "publicIdentifier": "indra82HffkBSTeSuvuEApLqLvY4wStnQYk7ysxZfYR2MmkwEe4XBbt", +
    //      "channelAddress": "0xaE55b9aDdcD1cF5F5ECbKff958F8C610d58f896d9",
    //      "amount": "1234", +
    //      "assetId": "0x0000000000000000000000000000000000000000", +
    //      "details": { +
    //        "lockHash": "0x40a089a80b03cd40d208cc383262fc3d6a8fa224ac8da55ea57998935ea73be5",
    //        "expiry": "0"
    //      },
    //      "recipient": "indra6RJffyPFY7D4pAkyooRxgXVyocxYkRUW3NFQVznHazn98vcadh", +

    payload["type"] = "HashlockTransfer";

    return _connextClient->transferCreate(payload).then([payload](QString transferId) mutable {
        payload["transferId"] = transferId;
        return payload;
    });
}

//==============================================================================

Promise<QString> ConnextSwapClient::getHubChannel()
{
    return getChannels().then([](QVector<QVariantMap> channels) {
        // this would be replaced by config value when hub is set up
        auto destination
            = "vector6At5HhbfhcE1p1SBTxAL5ej71AGWkSSxrSTmdj6wuFHquJ1hg8"; //_publicIdentifier.toStdString();

        for (auto&& channel : channels) {
            if (channel.value("aliceIdentifier").toString().toStdString() == destination
                || channel.value("bobIdentifier").toString().toStdString() == destination) {
                return channel.value("channelAddress").toString();
            }
        }
        return QString();
    });
}

//==============================================================================

uint32_t ConnextSwapClient::finalLock() const
{
    return 100;
}

//==============================================================================

ClientType ConnextSwapClient::type() const
{
    return ClientType::Connext;
}

//==============================================================================

double ConnextSwapClient::minutesPerBlock() const
{
    return 0.25;
}

//==============================================================================

uint32_t ConnextSwapClient::lockBuffer() const
{
    return static_cast<uint32_t>(std::round(LOCK_BUFFER_HOURS * 60 / minutesPerBlock()));
}

//==============================================================================

bool ConnextSwapClient::isConnected() const
{
    return true;
}

//==============================================================================

void ConnextSwapClient::onTransferResolved(ResolvedTransferResponse details)
{
    auto it = _proxies.find(details.lockHash);
    if (it != std::end(_proxies)) {
        if (auto proxy = it->second) {
            proxy->onResolveRequest(details);
            proxy->deleteLater();
        }
        _proxies.erase(it);
    }
}

//==============================================================================

Promise<QVector<QVariantMap>> ConnextSwapClient::getChannels()
{
    return destination().then([this](std::string dest) {
        auto publicIdentifier = QString::fromStdString(dest);
        return _connextClient->getChannelsAddresses(publicIdentifier)
            .then([this, publicIdentifier](std::vector<QString> channelsAddresses) {
                return QtPromise::map(
                    channelsAddresses, [publicIdentifier, this](QString channelAddress, ...) {
                        return _connextClient->getChannel(publicIdentifier, channelAddress);
                    });
            });
    });
}

//==============================================================================
}
