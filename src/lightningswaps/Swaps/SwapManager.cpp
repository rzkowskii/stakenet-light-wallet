#include "SwapManager.hpp"
#include <Orderbook/AbstractOrderbookClient.hpp>
#include <Swaps/AbstractSwapClient.hpp>
#include <Swaps/AbstractSwapClientPool.hpp>
#include <Swaps/AbstractSwapPeer.hpp>
#include <Swaps/AbstractSwapPeerPool.hpp>
#include <Swaps/AbstractSwapRepository.hpp>
#include <Swaps/Packets.hpp>
#include <Utils/Logging.hpp>
#include <hash.h>
#include <random.h>
#include <uint256.h>
#include <utilstrencodings.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QPointer>
#include <boost/math/distributions/poisson.hpp>
#include <cmath>

namespace swaps {

/** The maximum time in milliseconds we will wait for a swap to be accepted before failing it. */
static constexpr size_t SWAP_ACCEPT_TIMEOUT = 100000; // 5000;
/** The maximum time in milliseconds we will wait for a swap to be completed before failing it. */
static constexpr size_t SWAP_PAYMENT_RECEIVE_TIMEOUT = 100000; // 15000;
static constexpr size_t SWAP_INVOICE_EXCHANGE_TIMEOUT = 100000; // 10000;
static constexpr size_t SWAP_COMPLETED_TIMEOUT = 100000; // 5000;
/** The maximum time in milliseconds we will wait to receive an expected sanity swap init packet. */
static constexpr size_t SANITY_SWAP_INIT_TIMEOUT = 3000;
/** The maximum time in milliseconds we will wait for a swap to be completed before failing it. */
static constexpr size_t SANITY_SWAP_COMPLETE_TIMEOUT = 10000;

//==============================================================================

static QString ToString(SwapPhase phase)
{
    switch (phase) {
    case SwapPhase::SwapCreated:
        return "SwapCreated";
    case SwapPhase::SwapAccepted:
        return "SwapAccepted";
    case SwapPhase::SwapCompleted:
        return "SwapCompleted";
    case SwapPhase::SwapRequested:
        return "SwapRequested";
    case SwapPhase::SendingPayment:
        return "SendingPayment";
    case SwapPhase::SwapTakerCompleted:
        return "SwapTakerCompleted";
    case SwapPhase::InvoiceExchange:
        return "InvoiceExchange";
    case SwapPhase::PaymentReceived:
        return "PaymentReceived";
    default:
        return QString();
    }
}

//==============================================================================

// preimage + hash(preimage) in hex
static std::tuple<std::string, std::string> GeneratePreimage()
{
    std::vector<unsigned char> bytes(32);
    bitcoin::GetRandBytes(&bytes[0], static_cast<int>(bytes.size()));
    //    bitcoin::uint256 hash = bitcoin::Hash(bytes.begin(), bytes.end());

    //    {
    //        auto qhash = QCryptographicHash::hash(QByteArray::fromRawData((char*)&bytes[0],
    //        bytes.size()), QCryptographicHash::Sha256); auto parsed =
    //        bitcoin::ParseHex(bitcoin::HexStr(bytes)); Q_ASSERT(qhash ==
    //        QByteArray::fromRawData((char*)hash.begin(), hash.size()));
    //    }

    auto qhash = QCryptographicHash::hash(
        QByteArray::fromRawData((char*)&bytes[0], bytes.size()), QCryptographicHash::Sha256);

    return std::make_tuple(bitcoin::HexStr(bytes), qhash.toHex().toStdString());
}

//==============================================================================

static double PoissonQuantile(double numberOfEvents, double lambda)
{
    boost::math::poisson d(lambda);
    return boost::math::quantile(d, numberOfEvents);
}

//==============================================================================

/**
 * Calculates the minimum expected lock delta for the final hop of the first leg to ensure a
 * very high probability that it won't expire before the second leg payment. We use a Poisson
 * distribution to model the possible block times of two independent chains, first calculating
 * a probabilistic upper bound for the lock time in minuntes of the second leg then a
 * probabilistic lower bound for the number of blocks for the lock time extended to the final
 * hop of the first leg.
 * @param secondLegLockDuration The lock duration (aka time lock or cltv delta) of the second
 * leg (maker to taker) denominated in blocks of that chain.
 * @returns A number of blocks for the chain of the first leg that is highly likely to take
 * more time in minutes than the provided second leg lock duration.
 */
static int64_t CalculateLockBuffer(
    int64_t secondLegLockDuration, double secondLegMinutesPerBlock, double firstLegMinutesPerBlock)
{
    /** A probabilistic upper bound for the time it will take for the second leg route time lock to
     * expire. */
    const double secondLegLockMinutes
        = PoissonQuantile(.9999, secondLegLockDuration) * secondLegMinutesPerBlock;
    const double firstLegLockBuffer
        = PoissonQuantile(.9999, secondLegLockMinutes / firstLegMinutesPerBlock);

    return firstLegLockBuffer;
}

//==============================================================================

SwapManager::SwapManager(const AbstractSwapPeerPool& peers, AbstractSwapClientPool& clients,
    AbstractSwapRepository& repository, orderbook::AbstractOrderbookClient& orderbook,
    QObject* parent)
    : QObject(parent)
    , _peers(peers)
    , _clients(clients)
    , _repository(repository)
    , _orderbook(orderbook)
{
    connectSignals();
    // Load Swaps from database
    _repository.load();
    for (auto&& deal : _repository.deals()) {
        _usedHashes.insert(deal.rHash);
    }
}

//==============================================================================

Promise<SwapSuccess> SwapManager::executeSwap(PeerOrder maker, OwnOrder taker)
{
    QObject* context = new QObject;
    auto failBody = std::make_shared<packets::SwapFailedPacketBody>();
    return Promise<SwapSuccess>([=](const auto& resolve, const auto& reject) {
        try {
            this->verifyExecution(maker, taker);
            auto rHash = this->beginSwap(maker, taker);
            connect(this, &SwapManager::swapPaid, context, [resolve, rHash](SwapSuccess success) {
                if (rHash == success.rHash) {
                    resolve(success);
                }
            });

            connect(
                this, &SwapManager::swapFailed, context, [reject, rHash, failBody](SwapDeal deal) {
                    if (rHash == deal.rHash) {
                        SwapFailure failure;
                        failure.pairId = deal.pairId;
                        failure.orderId = deal.orderId;
                        failure.localId = deal.localId;
                        failure.quantity = deal.takerAmount;
                        failure.failureReason = deal.failureReason.get();
                        failure.failureMessage = deal.errorMessage.get_value_or({});
                        failure.role = deal.role;

                        failBody->set_rhash(rHash);
                        failBody->set_failurereason(static_cast<int>(failure.failureReason));
                        failBody->set_errormessage(failure.failureMessage);
                        reject(failure);
                    }
                });
        } catch (SwapException& ex) {
            SwapFailure failure;
            failure.pairId = taker.pairId;
            failure.orderId = taker.id;
            failure.localId = taker.localId;
            failure.role = SwapRole::Taker;
            failure.quantity = taker.quantity;
            failure.failureReason = ex.reason;
            failure.failureMessage = ex.what();

            qCCritical(Swaps) << "Failed to execute initial step for swap:"
                              << static_cast<int>(ex.reason) << ex.what()
                              << "for orderId:" << taker.id.c_str();

            failBody->set_rhash(taker.id);
            failBody->set_failurereason(static_cast<int>(ex.reason));
            reject(failure);
        }
    })
        .tapFail([failBody, this, pubkey = maker.peerPubKey] {
            try {
                _peers.peerByPubKey(pubkey)->sendPacket(SerializePacket(*failBody));
            } catch (std::exception& ex) {
                qCCritical(Swaps) << "Failed to send swap failure:" << ex.what();
            }
        })
        .finally([context] { context->deleteLater(); });
}

//==============================================================================

void SwapManager::timerEvent(QTimerEvent* event)
{
    auto it = std::find_if(std::begin(_timeouts), std::end(_timeouts),
        [id = event->timerId()](const auto& it) { return it.second.timerId == id; });

    if (it == std::end(_timeouts)) {
        return;
    }

    auto rHash = it->first;
    auto data = it->second;
    _timeouts.erase(it);

    try {
        auto deal = dealByHash(rHash);
        LogCCritical(Swaps) << "Swap timed out:" << deal.rHash.c_str()
                            << "reason:" << static_cast<int>(data.failReason);
        failDeal(deal, data.failReason);
    } catch (...) {
    }
}

//==============================================================================

/**
 * Handles a request from a peer to create a swap deal. Checks if the order for the requested swap
 * is available and if a route exists to determine if the request should be accepted or rejected.
 * Responds to the peer with a swap response packet containing either an accepted quantity or
 * rejection reason.
 */
void SwapManager::onSwapRequested(
    packets::SwapRequestPacketBody requestPacket, QPointer<AbstractSwapPeer> peer)
{
    LogCDebug(Swaps) << "Received swap request from peer:" << peer->nodePubKey().c_str()
                     << "orderId:" << requestPacket.orderid().c_str()
                     << "deal: " << requestPacket.rhash().c_str();

    //    if (!Swaps.validateSwapRequest(requestPacket.body!)) {
    //      // TODO: penalize peer for invalid swap request
    //      await peer.sendPacket(new SwapFailedPacket({
    //        rHash,
    //        failureReason: SwapFailureReason::InvalidSwapRequest,
    //      }, requestPacket.header.id));
    //      return;
    //    }

    packets::SwapFailedPacketBody failBody;
    failBody.set_rhash(requestPacket.rhash());
    failBody.set_failurereason(static_cast<int>(SwapFailureReason::UnknownError));

    try {
        auto order = _orderbook.getOwnOrder(requestPacket.pairid(), requestPacket.orderid());
        const auto availableQuantity = order.quantity - order.hold;
        if (availableQuantity > 0) {
            const auto quantity = std::min(requestPacket.proposedquantity(), availableQuantity);
            // try to accept the deal
            OrderToAccept orderToAccept;
            orderToAccept.isBuy = order.isBuy;
            orderToAccept.price = order.price;
            orderToAccept.quantity = quantity;
            orderToAccept.localId = order.localId;

            auto accepted = acceptDeal(orderToAccept, requestPacket, peer);

            LogCDebug(Swaps) << "Deal with id:" << requestPacket.rhash().c_str()
                             << "accepted:" << accepted;
            if (!accepted) {
                removeOrderHold(order.id, requestPacket.pairid(), quantity);
            }

            return;
        }
    } catch (SwapException& ex) {
        LogCCritical(Swaps) << "Swap request:" << requestPacket.rhash().c_str()
                            << "failed with swap specific error" << static_cast<int>(ex.reason)
                            << ex.what();

        failBody.set_failurereason(static_cast<uint32_t>(ex.reason));
        failBody.set_errormessage(ex.what());
    } catch (std::exception& ex) {
        LogCCritical(Swaps) << "Swap request:" << requestPacket.rhash().c_str()
                            << "failed with general error:" << ex.what();
        failBody.set_errormessage(ex.what());
    }

    peer->sendPacket(SerializePacket(failBody));
}

//==============================================================================

void SwapManager::onSwapAccepted(
    packets::SwapAcceptedPacketBody body, QPointer<AbstractSwapPeer> peer)
{
    auto job = [&] {
        if (!_deals.count(body.rhash())) {
            LogCWarning(Swaps) << "received swap accepted for unrecognized deal:"
                               << body.rhash().c_str();
            // TODO: penalize peer
            return;
        }

        auto deal = dealByHash(body.rhash());

        deal.destination = body.makerdestination();

        if (deal.phase != SwapPhase::SwapRequested) {
            LogCWarning(Swaps)
                << "received swap accepted for deal that is not in SwapRequested phase:"
                << body.rhash().c_str();
            // TODO: penalize peer
            return;
        }

        // clear the timer waiting for acceptance of our swap offer, and set a new timer waiting for
        // the swap to be completed
        clearTimeout(deal.rHash);

        // update deal with maker's cltv delta
        deal.makerCltvDelta = body.makercltvdelta();

        auto quantity = body.quantity();
        if (quantity) {
            deal.quantity = quantity; // set the accepted quantity for the deal
            if (quantity <= 0) {
                failDeal(deal, SwapFailureReason::InvalidSwapPacketReceived,
                    std::string("accepted quantity must be a positive number"));
                throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
                // TODO: penalize peer
            } else if (quantity > deal.proposedQuantity) {
                failDeal(deal, SwapFailureReason::InvalidSwapPacketReceived,
                    std::string("accepted quantity should not be greater than proposed quantity"));
                throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
                // TODO: penalize peer
            } else if (quantity < deal.proposedQuantity) {
                auto amounts = MakerTakerAmounts::CalculateMakerTakerAmounts(
                    quantity, deal.price, deal.isBuy, deal.pairId);
                deal.takerAmount = amounts.taker.amount;
                deal.makerAmount = amounts.maker.amount;
            }
        }

        deal.paymentRequest = body.makerpaymentrequest();

        const auto takerSwapClient = _clients.getClient(deal.takerCurrency);
        std::string paymentRequest;
        {
            auto promise
                = takerSwapClient->addHodlInvoice(deal.rHash, deal.takerUnits, deal.takerCltvDelta)
                      .tap([&paymentRequest](std::string req) { paymentRequest = req; })
                      .wait();
            if (promise.isRejected()) {
                failDeal(deal, SwapFailureReason::UnexpectedClientError,
                    std::string("failed to add taker invoice"));
                throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
            }
        }

        // persist the deal to the database before we attempt to send
        setDealPhase(deal, SwapPhase::InvoiceExchange);

        packets::InvoiceExchangePacketBody responseBody;
        responseBody.set_rhash(deal.rHash);
        responseBody.set_takerpaymentrequest(paymentRequest);

        QPointer<SwapManager> self(this);
        peer->sendPacket(SerializePacket(responseBody)).tapFail([self, deal]() mutable {
            if (self) {
                self->failDeal(deal, SwapFailureReason::RemoteError,
                    std::string("failed to send invoice exchange packet"));
            }
        });
    };

    packets::SwapFailedPacketBody failBody;
    failBody.set_rhash(body.rhash());
    failBody.set_failurereason(static_cast<int>(SwapFailureReason::UnknownError));

    try {
        job();
        return;
    } catch (SwapException& ex) {
        LogCCritical(Swaps) << "Swap accepted:" << body.rhash().c_str()
                            << "failed with swap specific error:" << static_cast<int>(ex.reason)
                            << ex.what();
        failBody.set_failurereason(static_cast<uint32_t>(ex.reason));
        failBody.set_errormessage(ex.what());
    } catch (std::exception& ex) {
        LogCCritical(Swaps) << "Swap accepted:" << body.rhash().c_str()
                            << "failed with general error:" << ex.what();
        failBody.set_errormessage(ex.what());
    }

    peer->sendPacket(SerializePacket(failBody));
}

//==============================================================================

void SwapManager::onInvoiceExchange(
    packets::InvoiceExchangePacketBody body, QPointer<AbstractSwapPeer> peer)
{
    auto job =
        [&] {
            if (!_deals.count(body.rhash())) {
                LogCWarning(Swaps)
                    << "received swap accepted for unrecognized deal:" << body.rhash().c_str();
                // TODO: penalize peer
                return;
            }

            auto deal = dealByHash(body.rhash());

            clearTimeout(deal.rHash);

            if (deal.phase != SwapPhase::SwapAccepted) {
                LogCWarning(Swaps)
                    << "received swap invoice exchange for deal that is not in SwapAccepted phase:"
                    << body.rhash().c_str();
                // TODO: penalize peer
                return;
            }

            deal.paymentRequest = body.takerpaymentrequest();

            setDealPhase(deal, SwapPhase::InvoiceExchange);

            packets::InvoiceExchangeAckPacketBody response;
            response.set_rhash(deal.rHash);

            LogCDebug(Swaps) << "Sending invoice exchange ack to peer"
                             << peer->nodePubKey().c_str();
            setTimeout(deal.rHash, SWAP_PAYMENT_RECEIVE_TIMEOUT, SwapFailureReason::SwapTimedOut);
            QPointer<SwapManager> self(this);
            peer->sendPacket(SerializePacket(response)).tapFail([self, deal]() mutable {
                if (self) {
                    self->failDeal(deal, SwapFailureReason::RemoteError,
                        std::string("failed to send invoice exchange ack packet"));
                }
            });
        };

    packets::SwapFailedPacketBody failBody;
    failBody.set_rhash(body.rhash());
    failBody.set_failurereason(static_cast<int>(SwapFailureReason::UnknownError));

    try {
        job();
        return;
    } catch (SwapException& ex) {
        LogCCritical(Swaps) << "Swap accepted:" << body.rhash().c_str()
                            << "failed with swap specific error:" << static_cast<int>(ex.reason)
                            << ex.what();
        failBody.set_failurereason(static_cast<uint32_t>(ex.reason));
        failBody.set_errormessage(ex.what());
    } catch (std::exception& ex) {
        LogCCritical(Swaps) << "Swap accepted:" << body.rhash().c_str()
                            << "failed with general error:" << ex.what();
        failBody.set_errormessage(ex.what());
    }

    peer->sendPacket(SerializePacket(failBody));
}

//==============================================================================

void SwapManager::onInvoiceExchangeAck(
    packets::InvoiceExchangeAckPacketBody body, QPointer<AbstractSwapPeer> peer)
{
    auto job = [&] {
        if (!_deals.count(body.rhash())) {
            LogCWarning(Swaps) << "received swap accepted for unrecognized deal:"
                               << body.rhash().c_str();
            // TODO: penalize peer
            return;
        }

        auto deal = dealByHash(body.rhash());

        if (deal.phase != SwapPhase::InvoiceExchange) {
            LogCWarning(Swaps) << "received swap invoice exchange ack for deal that is not in "
                                  "InvoiceExchange phase:"
                               << body.rhash().c_str();
            // TODO: penalize peer
            return;
        }

        // clear the timer waiting for acceptance of our swap offer, and set a new timer waiting for
        // the swap to be completed
        clearTimeout(deal.rHash);

        setDealPhase(deal, SwapPhase::SendingPayment);

        LogCCInfo(Swaps) << "Successfully exchanged invoices, sending payment to maker, deal"
                         << deal.rHash.c_str();

        const auto makerSwapClient = _clients.getClient(deal.makerCurrency);
        {
            std::string sendPaymentError;
            auto promise = makerSwapClient->sendPayment(deal)
                               .tapFail([&sendPaymentError](const std::exception& error) {
                                   sendPaymentError = error.what();
                               })
                               .wait();
            if (promise.isRejected()) {
                failDeal(deal, SwapFailureReason::SendPaymentFailure, sendPaymentError);
                throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
            }
        }

        deal = dealByHash(deal.rHash);

        packets::SwapCompletePacketBody responseBody;

        // swap succeeded!
        responseBody.set_rhash(body.rhash());

        LogCDebug(Swaps) << "Sending swap complete to peer" << peer->nodePubKey().c_str()
                         << "rHash: " << deal.rHash.c_str();
        setDealPhase(deal, SwapPhase::SwapCompleted);
        peer->sendPacket(SerializePacket(responseBody));
        //            failDeal(deal, SwapFailureReason::RemoteError, std::string("failed to send
        //            swap complete packet")); throw SwapException(deal.failureReason.get(),
        //            deal.errorMessage.get());
    };

    packets::SwapFailedPacketBody failBody;
    failBody.set_rhash(body.rhash());
    failBody.set_failurereason(static_cast<int>(SwapFailureReason::UnknownError));

    try {
        job();
        return;
    } catch (SwapException& ex) {
        LogCCritical(Swaps) << "Swap accepted:" << body.rhash().c_str()
                            << "failed with swap specific error:" << static_cast<int>(ex.reason)
                            << ex.what();
        failBody.set_failurereason(static_cast<uint32_t>(ex.reason));
        failBody.set_errormessage(ex.what());
    } catch (std::exception& ex) {
        LogCCritical(Swaps) << "Swap accepted:" << body.rhash().c_str()
                            << "failed with general error:" << ex.what();
        failBody.set_errormessage(ex.what());
    }

    peer->sendPacket(SerializePacket(failBody));
}

//==============================================================================

void SwapManager::onSwapCompleted(
    packets::SwapCompletePacketBody body, QPointer<AbstractSwapPeer> /*peer*/)
{
    if (_deals.count(body.rhash()) > 0) {
        setDealPhase(_deals.at(body.rhash()), SwapPhase::SwapTakerCompleted);
    } else {
        LogCDebug(Swaps) << "received swap complete for unknown deal payment hash"
                         << body.rhash().c_str();
    }
}

//==============================================================================

void SwapManager::onSwapFailed(packets::SwapFailedPacketBody body, QPointer<AbstractSwapPeer> peer)
{
    LogCCritical(Swaps) << "Peer swap failed, rhash:" << body.rhash().c_str()
                        << "reason:" << body.failurereason()
                        << ", details:" << body.errormessage().c_str();
    try {
        auto deal = dealByHash(body.rhash());
        failDeal(deal, SwapFailureReason::RemoteError, body.errormessage());
    } catch (...) {
        if (auto dealInstance = _repository.deal(body.rhash())) {
            if (dealInstance->state == SwapState::Error
                && dealInstance->failureReason == SwapFailureReason::RemoteError) {
                auto errorMessageWithReason
                    = std::to_string(body.failurereason()) + " - " + body.errormessage();
                // update the error message for this saved deal to include the reason it failed
                dealInstance->errorMessage = errorMessageWithReason + " - "
                    + dealInstance->errorMessage.get_value_or("no-message");
                _repository.updateDeal(dealInstance.get());
            } else {
                LogCWarning(Orderbook)
                    << "received unexpected swap failed packet for deal with payment hash"
                    << body.rhash().c_str();
            }
        } else {
            LogCWarning(Orderbook)
                << "received swap failed packet for unknown deal with payment hash"
                << body.rhash().c_str();
        }
    }
}

//==============================================================================

void SwapManager::onHtlcAccepted(
    std::string rHash, int64_t amount, std::string currency, QVariantMap payload)
{
    try {
        auto rPreimage = resolveHash(rHash, amount, currency);
        auto swapClient = _clients.getClient(currency);
        auto promise = swapClient->settleInvoice(rHash, rPreimage, payload).wait();
        if (promise.isRejected()) {
            throw SwapException(SwapFailureReason::SendPaymentFailure);
        }

        auto deal = dealByHash(rHash);
        deal.rPreimage = rPreimage;
        setDealPhase(deal, SwapPhase::PaymentReceived);
        if (deal.role == SwapRole::Maker) {
            auto deal = dealByHash(rHash);
            setDealPhase(deal, SwapPhase::SwapCompleted);
        }

    } catch (std::exception& ex) {
        LogCCritical(Swaps) << "could not settle invoice" << rHash.c_str() << ex.what();
    }
}

//==============================================================================

std::string SwapManager::beginSwap(PeerOrder maker, OwnOrder taker)
{
    AbstractSwapPeer& peer = peerByPubKey(maker.peerPubKey);

    auto quantity = std::min(maker.quantity, taker.quantity);
    const auto amounts = MakerTakerAmounts::CalculateMakerTakerAmounts(
        quantity, maker.price, maker.isBuy, maker.pairId);
    const auto takerClient = _clients.getClient(amounts.taker.currency);

    const auto takerCltvDelta = takerClient->finalLock();

    std::string rPreimage, rHash;
    do {
        std::tie(rPreimage, rHash) = GeneratePreimage();
    } while (_usedHashes.count(rHash) > 0);

    packets::SwapRequestPacketBody body;
    body.set_orderid(maker.id);
    body.set_pairid(maker.pairId);
    body.set_proposedquantity(taker.quantity);
    body.set_rhash(rHash);
    body.set_takercltvdelta(takerCltvDelta);

    SwapDeal deal = ConstructWith<SwapDeal>(body);

    deal.takerCurrency = amounts.taker.currency;
    deal.makerCurrency = amounts.maker.currency;
    deal.takerAmount = amounts.taker.amount;
    deal.makerAmount = amounts.maker.amount;
    deal.takerUnits = amounts.taker.units;
    deal.makerUnits = amounts.maker.units;
    deal.rPreimage = rPreimage;
    deal.peerPubKey = peer.nodePubKey();
    deal.localId = taker.localId;
    deal.price = maker.price;
    deal.isBuy = maker.isBuy;
    deal.phase = SwapPhase::SwapCreated;
    deal.state = SwapState::Active;
    deal.role = SwapRole::Taker;
    deal.quantity = quantity;
    deal.createTime = QDateTime::currentMSecsSinceEpoch();
    deal.orderType = taker.type;

    addDeal(deal);

    // Make sure we are connected to both swap clients
    if (!isPairSupported(deal.pairId)) {
        failDeal(deal, SwapFailureReason::SwapClientNotSetup);
        throw SwapException(SwapFailureReason::SwapClientNotSetup);
    }

    {
        std::string destination;
        auto promise = takerClient->destination()
                           .tap([&destination](std::string dest) { destination = dest; })
                           .wait();
        if (promise.isRejected()) {
            failDeal(deal, SwapFailureReason::SwapClientNotSetup);
            throw SwapException(SwapFailureReason::SwapClientNotSetup);
        }

        body.set_takerdestination(destination);
        deal.takerPubKey = destination;
    }

    LogCCInfo(Swaps) << "Begining swap with peer:" << peer.nodePubKey().c_str()
                     << "rHash:" << rHash.c_str() << "orderId:" << body.orderid().c_str()
                     << "amounts:" << amounts.toString().c_str();

    setTimeout(rHash, SWAP_ACCEPT_TIMEOUT, SwapFailureReason::DealTimedOut);
    setDealPhase(deal, SwapPhase::SwapRequested);

    QPointer<SwapManager> self(this);
    peer.sendPacket(SerializePacket(body)).tapFail([self, deal]() mutable {
        if (self) {
            self->failDeal(deal, SwapFailureReason::RemoteError,
                std::string("Failed to send SwapRequestPacketBody packet"));
        }
    });

    return deal.rHash;
}

//==============================================================================

void SwapManager::verifyExecution(PeerOrder maker, OwnOrder taker)
{
    if (maker.pairId != taker.pairId || !isPairSupported(maker.pairId)) {
        throw SwapException(SwapFailureReason::SwapClientNotSetup);
    }

    const auto amounts = MakerTakerAmounts::CalculateMakerTakerAmounts(
        taker.quantity, maker.price, maker.isBuy, maker.pairId);

    _clients.getClient(amounts.maker.currency);
    auto peer = _peers.peerByPubKey(maker.peerPubKey);

    if (!peer) {
        throw SwapException(SwapFailureReason::RemoteError);
    }
}

//==============================================================================

AbstractSwapPeer& SwapManager::peerByPubKey(std::string pubKey) const
{
    if (auto peer = _peers.peerByPubKey(pubKey)) {
        return *peer;
    }

    throw SwapException(SwapFailureReason::RemoteError);
}

//==============================================================================

void SwapManager::setDealPhase(SwapDeal deal, SwapPhase newPhase)
{
    // nothing to do if swap was already completed
    if (deal.phase == SwapPhase::SwapCompleted || deal.state == SwapState::Completed) {
        return;
    }

    Q_ASSERT_X(
        deal.state == SwapState::Active, __func__, "deal is not Active. Can not change deal phase");

    LogCCInfo(Swaps) << "Changing deal state:" << deal.rHash.c_str() << ToString(deal.phase) << "->"
                     << ToString(newPhase);

    switch (newPhase) {
    case SwapPhase::SwapCreated:
        Q_ASSERT_X(false, __func__, "can not set deal phase to SwapCreated.");
        break;
    case SwapPhase::SwapRequested:
        Q_ASSERT_X(
            deal.role == SwapRole::Taker, __func__, "SwapRequested can only be set by the taker");
        Q_ASSERT_X(deal.phase == SwapPhase::SwapCreated, __func__,
            "SwapRequested can be only be set after SwapCreated");
        break;
    case SwapPhase::SwapAccepted:
        Q_ASSERT_X(
            deal.role == SwapRole::Maker, __func__, "SwapAccepted can only be set by the maker");
        Q_ASSERT_X(deal.phase == SwapPhase::SwapCreated, __func__,
            "SwapAccepted can be only be set after SwapCreated");
        break;
    case SwapPhase::SendingPayment:
        Q_ASSERT_X(deal.phase == SwapPhase::InvoiceExchange, __func__,
            "SendingPayment can only be set after InvoiceExchange");
        deal.executeTime = QDateTime::currentMSecsSinceEpoch();
        break;
    case SwapPhase::PaymentReceived:
        Q_ASSERT_X(
            deal.phase == SwapPhase::SendingPayment || deal.phase == SwapPhase::SwapTakerCompleted,
            __func__, "PaymentReceived can be only be set after SendingPayment");
        LogCDebug(Swaps) << "Payment received for deal with payment hash"
                         << deal.rPreimage.get_value_or(std::string()).c_str();
        if (deal.phase == SwapPhase::SwapTakerCompleted) {
            setDealPhase(deal, SwapPhase::SwapCompleted);
            return;
        }
        break;
    case SwapPhase::InvoiceExchange:
        Q_ASSERT_X((deal.role == SwapRole::Taker && deal.phase == SwapPhase::SwapRequested)
                || (deal.role == SwapRole::Maker && deal.phase == SwapPhase::SwapAccepted),
            __func__,
            "InvoiceExchange can only be set after SwapRequested (taker) or SwapAccepted (maker)");
        break;
    case SwapPhase::SwapTakerCompleted:
        Q_ASSERT_X(deal.role == SwapRole::Maker, __func__,
            "SwapTakerCompleted can only be set by the maker");
        // if we have received payment then we need to change state to SwapCompleted
        if (deal.phase == SwapPhase::PaymentReceived) {
            setDealPhase(deal, SwapPhase::SwapCompleted);
            return;
        }
        break;
    case SwapPhase::SwapCompleted:
        Q_ASSERT_X(
            deal.phase == SwapPhase::PaymentReceived || deal.phase == SwapPhase::SwapTakerCompleted,
            __func__,
            "SwapCompleted can be only be set after PaymentReceived or SwapTakerCompleted");
        deal.completeTime = QDateTime::currentMSecsSinceEpoch();
        deal.state = SwapState::Completed;
        LogCDebug(Swaps) << "Swap completed. preimage ="
                         << deal.rPreimage.get_value_or(std::string()).c_str();
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    deal.phase = newPhase;
    _deals[deal.rHash] = deal;

    if (deal.phase != SwapPhase::SwapCreated && deal.phase != SwapPhase::SwapRequested) {
        // once a deal is accepted, we persist its state to the database on every phase update
        persistDeal(deal);
    }

    if (deal.phase == SwapPhase::PaymentReceived) {
        QPointer<SwapManager> self(this);
        QTimer::singleShot(SWAP_COMPLETED_TIMEOUT, this, [self, rHash = deal.rHash] {
            if (!self || self->_deals.count(rHash) == 0) {
                return;
            }

            auto& deal = self->dealByHash(rHash);
            if (deal.phase == SwapPhase::PaymentReceived) {
                LogCCInfo(Swaps) << "Deal" << rHash.c_str()
                                 << "still at SwapPhase::PaymentReceived state, moving it to "
                                    "SwapCompleted state. Never received SwapCompleted";
                self->setDealPhase(deal, SwapPhase::SwapCompleted);
            }
        });

        clearTimeout(deal.rHash);
    }

    if (deal.phase == SwapPhase::SwapCompleted) {
        const auto wasMaker = deal.role == SwapRole::Maker;
        SwapSuccess swapSuccess;
        swapSuccess.orderId = deal.orderId;
        swapSuccess.localId = deal.localId;
        swapSuccess.pairId = deal.pairId;
        swapSuccess.quantity = deal.quantity.get();
        swapSuccess.amountReceived = wasMaker ? deal.makerAmount : deal.takerAmount;
        swapSuccess.amountSent = wasMaker ? deal.takerAmount : deal.makerAmount;
        swapSuccess.currencyReceived = wasMaker ? deal.makerCurrency : deal.takerCurrency;
        swapSuccess.currencySent = wasMaker ? deal.takerCurrency : deal.makerCurrency;
        swapSuccess.rHash = deal.rHash;
        swapSuccess.rPreimage = deal.rPreimage.get();
        swapSuccess.price = deal.price;
        swapSuccess.peerPubKey = deal.peerPubKey;
        swapSuccess.role = deal.role;
        swapSuccess.type = deal.orderType;
        swapPaid(swapSuccess);
        clearTimeout(deal.rHash);
    }
}

//==============================================================================

void SwapManager::persistDeal(SwapDeal deal)
{
    _repository.saveDeal(deal);
    if (deal.state != SwapState::Active) {
        _deals.erase(deal.rHash);
    }
}

//==============================================================================

void SwapManager::addDeal(SwapDeal deal)
{
    _deals.emplace(deal.rHash, deal);
    _usedHashes.emplace(deal.rHash);
    LogCDebug(Swaps) << "new deal" << deal.rHash.c_str();
}

//==============================================================================

void SwapManager::removeOrderHold(std::string orderId, std::string pairId, int64_t quantity)
{
    LogCDebug(Swaps) << "Removing order:" << orderId.c_str() << "for pair:" << pairId.c_str()
                     << "quantity:" << quantity;
}

//==============================================================================

SwapDeal& SwapManager::dealByHash(std::string rHash)
{
    if (_deals.count(rHash) > 0) {
        return _deals.at(rHash);
    }

    throw SwapException(SwapFailureReason::UnknownError, "deal with hash: " + rHash + " not found");
}

//==============================================================================

/**
 * Checks if there are connected swap clients for both currencies in a given trading pair.
 * @returns `true` if the pair has swap support, `false` otherwise
 */
bool SwapManager::isPairSupported(std::string pairId) const
{
    const auto currencies = QString::fromStdString(pairId).split('_');
    const auto baseCurrencyClient = _clients.getClient(currencies[0].toStdString());
    const auto quoteCurrencyClient = _clients.getClient(currencies[1].toStdString());
    return (baseCurrencyClient && baseCurrencyClient->isConnected())
        && (quoteCurrencyClient && quoteCurrencyClient->isConnected());
}

//==============================================================================

void SwapManager::connectSignals()
{
    auto peers = &_peers;
    connect(peers, &AbstractSwapPeerPool::swapRequestReceived, this, &SwapManager::onSwapRequested);
    connect(peers, &AbstractSwapPeerPool::swapAcceptedReceived, this, &SwapManager::onSwapAccepted);
    connect(peers, &AbstractSwapPeerPool::swapInvoiceExchangeReceived, this,
        &SwapManager::onInvoiceExchange);
    connect(peers, &AbstractSwapPeerPool::swapInvoiceExchangeAckReceived, this,
        &SwapManager::onInvoiceExchangeAck);
    connect(
        peers, &AbstractSwapPeerPool::swapCompleteReceived, this, &SwapManager::onSwapCompleted);
    connect(peers, &AbstractSwapPeerPool::swapFailReceived, this, &SwapManager::onSwapFailed);
    connect(&_clients, &AbstractSwapClientPool::htlcAccepted, this, &SwapManager::onHtlcAccepted);
    connect(&_clients, &AbstractSwapClientPool::resolveRequestReady, this,
        &SwapManager::handleResolveRequest);
}

//==============================================================================

void SwapManager::failDeal(
    SwapDeal& deal, SwapFailureReason reason, boost::optional<std::string> errorMessage)
{
    Q_ASSERT_X(deal.state != SwapState::Completed, __func__, "Can not fail a completed deal.");

    // If we are already in error state and got another error report we
    // aggregate all error reasons by concatenation
    if (deal.state == SwapState::Error) {
        if (errorMessage) {
            deal.errorMessage = deal.errorMessage
                ? deal.errorMessage.get() + errorMessage.get_value_or(std::string())
                : errorMessage;
        }
        LogCCritical(Swaps) << "new deal error message for ${deal.rHash}: + ${deal.errorMessage}";
        return;
    }

    LogCCritical(Swaps) << "deal" << deal.rHash.c_str() << "failed due to"
                        << static_cast<int>(reason)
                        << errorMessage.get_value_or(std::string()).c_str();

    switch (reason) {
    case SwapFailureReason::SwapTimedOut:
        // additional penalty as timeouts cause costly delays and possibly stuck HTLC outputs
        //        void this.pool.addReputationEvent(deal.peerPubKey, ReputationEvent.SwapTimeout);
        /* falls through */
    case SwapFailureReason::SendPaymentFailure:
    case SwapFailureReason::NoRouteFound:
    case SwapFailureReason::SwapClientNotSetup:
        // something is wrong with swaps for this trading pair and peer, drop this pair
        //        try {
        // TODO: disable the currency that caused this error
        //          this.pool.getPeer(deal.peerPubKey).deactivatePair(deal.pairId);
        //        } catch (err) {
        //          LogCDebug(Swaps) << (`could not drop trading pair ${deal.pairId} for peer
        //          ${deal.peerPubKey}`);
        //        }
        //        void this.pool.addReputationEvent(deal.peerPubKey, ReputationEvent.SwapFailure);
        break;
    case SwapFailureReason::InvalidResolveRequest:
    case SwapFailureReason::DealTimedOut:
    case SwapFailureReason::InvalidSwapPacketReceived:
    case SwapFailureReason::PaymentHashReuse:
        // peer misbehaving, penalize the peer
        //        void this.pool.addReputationEvent(deal.peerPubKey,
        //        ReputationEvent.SwapMisbehavior);
        break;
    default:
        // do nothing, the swap failed for an innocuous reason
        break;
    }

    deal.state = SwapState::Error;
    deal.completeTime = QDateTime::currentMSecsSinceEpoch();
    deal.failureReason = reason;
    deal.errorMessage = errorMessage;

    qCCritical(Orderbook) << "Failing deal" << deal.rHash.c_str()
                          << static_cast<int>(deal.failureReason.get())
                          << deal.errorMessage.get_value_or("").c_str();

    if (deal.phase != SwapPhase::SwapCreated && deal.phase != SwapPhase::SwapRequested) {
        // persist the deal failure if it had been accepted
        persistDeal(deal);
    } else {
        _deals.erase(deal.rHash);
    }

    clearTimeout(deal.rHash);

    try {
        const auto swapClient = _clients.getClient(
            deal.role == SwapRole::Maker ? deal.makerCurrency : deal.takerCurrency);

        // Allow to remove invoice only in case swap payments were not started yet.
        if (deal.phase == SwapPhase::SwapCreated || deal.phase == SwapPhase::SwapAccepted
            || deal.phase == SwapPhase::SwapRequested) {
            swapClient->removeInvoice(deal.rHash); // we don't need to await the remove invoice call
        }
    } catch (...) {
    }

    this->swapFailed(deal);
}

//==============================================================================

bool SwapManager::acceptDeal(OrderToAccept orderToAccept, packets::SwapRequestPacketBody body,
    QPointer<AbstractSwapPeer> peer)
{
    // TODO: max cltv to limit routes
    // TODO: consider the time gap between taking the routes and using them.

    auto rHash = body.rhash();
    if (_usedHashes.count(rHash) > 0) {
        throw SwapException(SwapFailureReason::PaymentHashReuse, "Payment hash reuse");
    }

    const auto amounts = MakerTakerAmounts::CalculateMakerTakerAmounts(
        orderToAccept.quantity, orderToAccept.price, orderToAccept.isBuy, body.pairid());

    LogCCInfo(Swaps) << "Trying to accept deal:" << orderToAccept.localId.c_str()
                     << "from:" << peer->nodePubKey().c_str() << "rHash:" << body.rhash().c_str()
                     << "amounts:" << amounts.toString().c_str();

    AbstractSwapClient* makerSwapClient{ nullptr };
    try {
        makerSwapClient = _clients.getClient(amounts.maker.currency);
    } catch (std::exception&) {
        throw SwapException(SwapFailureReason::SwapClientNotSetup, "Unsupported maker currency");
    }

    AbstractSwapClient* takerSwapClient{ nullptr };
    try {
        takerSwapClient = _clients.getClient(amounts.taker.currency);
    } catch (std::exception&) {
        throw SwapException(SwapFailureReason::SwapClientNotSetup, "Unsupported taker currency");
    }

    const auto takerIdentifier = body.takerdestination();

    auto deal = ConstructWith<SwapDeal>(body);
    deal.price = orderToAccept.price;
    deal.isBuy = orderToAccept.isBuy;
    deal.quantity = orderToAccept.quantity;
    deal.makerAmount = amounts.maker.amount;
    deal.takerAmount = amounts.taker.amount;
    deal.makerUnits = amounts.maker.units;
    deal.takerPubKey = takerIdentifier;
    deal.destination = takerIdentifier;
    deal.takerUnits = amounts.taker.units;
    deal.makerCurrency = amounts.maker.currency;
    deal.takerCurrency = amounts.taker.currency;
    deal.localId = orderToAccept.localId;
    deal.phase = SwapPhase::SwapCreated;
    deal.state = SwapState::Active;
    deal.role = SwapRole::Maker;
    deal.createTime = QDateTime::currentMSecsSinceEpoch();
    deal.rHash = rHash;

    // add the deal. Going forward we can "record" errors related to this deal.
    addDeal(deal);

    // Make sure we are connected to swap clients for both currencies
    if (!isPairSupported(deal.pairId)) {
        failDeal(deal, SwapFailureReason::SwapClientNotSetup);
        throw SwapException(
            SwapFailureReason::SwapClientNotSetup, deal.errorMessage.get_value_or({}));
    }

    AbstractSwapClient::Routes makerToTakerRoutes;

    {
        std::string errMsg;
        auto promise
            = takerSwapClient
                  ->getRoutes(1000, takerIdentifier, deal.takerCurrency, deal.takerCltvDelta)
                  .tapFail([&errMsg](const std::exception& errorMsg) { errMsg = errorMsg.what(); })
                  .tap([&makerToTakerRoutes](
                           AbstractSwapClient::Routes routes) { makerToTakerRoutes = routes; });
        if (promise.wait().isRejected()) {
            if (errMsg.find("destination hops disabled") != std::string::npos) {
                failDeal(deal, SwapFailureReason::NoRouteFound,
                    std::string(
                        "The LN network graph is waiting for updates, please wait for it to "
                        "complete before placing an order. This may take up to 15 minutes."));
                throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
            }

            failDeal(deal, SwapFailureReason::NoRouteFound,
                std::string("Currently there is no route to fit this order volume, please try "
                            "smaller amounts"));
            throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
        }
    }

    if (makerToTakerRoutes.empty()) {
        failDeal(deal, SwapFailureReason::NoRouteFound,
            std::string("Currently there is no route to fit this order volume, please try smaller "
                        "amounts"));
        throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
    }

    uint32_t height;

    {
        auto promise = takerSwapClient->getHeight().tap([&height](uint32_t h) { height = h; });
        if (promise.wait().isRejected()) {
            failDeal(deal, SwapFailureReason::UnexpectedClientError,
                std::string("Unable to fetch block height"));
            throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
        }
    }

    std::string makerDestination;

    {
        auto promise = makerSwapClient->destination().tap(
            [&makerDestination](std::string dest) { makerDestination = dest; });
        if (promise.wait().isRejected()) {
            failDeal(deal, SwapFailureReason::UnexpectedClientError,
                std::string("Unable to fetch maker destination"));
            throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
        }
    }

    deal.makerPubKey = makerDestination;

    LogCDebug(Swaps) << "Got" << amounts.taker.currency.c_str() << "block height of" << height;

    const auto routeAbsoluteTimeLock = makerToTakerRoutes.at(0).total_time_lock;
    const auto routeLockDuration = routeAbsoluteTimeLock - height;
    const auto routeLockHours
        = std::round((routeLockDuration * takerSwapClient->minutesPerBlock()) / 60.0);
    LogCDebug(Swaps) << "Found route to taker with total lock duration of" << routeLockDuration
                     << amounts.taker.currency.c_str() << "blocks" << routeLockHours;
    // Add an additional buffer equal to our final lock to allow for more possible routes.
    deal.takerMaxTimeLock = routeLockDuration + takerSwapClient->finalLock();

    const auto makerClientLockBuffer = CalculateLockBuffer(deal.takerMaxTimeLock.value(),
        takerSwapClient->minutesPerBlock(), makerSwapClient->minutesPerBlock());
    //    const auto makerClientLockBuffer = makerSwapClient->lockBuffer();
    const auto makerClientLockBufferHours
        = std::round((makerClientLockBuffer * makerSwapClient->minutesPerBlock()) / 60.0);
    LogCDebug(Swaps) << "calculated lock buffer for first leg:" << makerClientLockBuffer
                     << amounts.maker.currency.c_str() << "blocks" << makerClientLockBufferHours;

    // Here we calculate the minimum lock delta we will expect as maker on the final hop to us on
    // the first leg of the swap. This should ensure a very high probability that the final hop
    // of the payment to us won't expire before our payment to the taker with time leftover to
    // satisfy our finalLock/cltvDelta requirement for the incoming payment swap client.
    deal.makerCltvDelta = makerClientLockBuffer + makerSwapClient->finalLock();
    const auto makerCltvDeltaHours
        = std::round((deal.makerCltvDelta.get() * makerSwapClient->minutesPerBlock()) / 60.0);
    LogCDebug(Swaps) << "calculated lock delta for final hop to maker:" << deal.makerCltvDelta.get()
                     << amounts.maker.currency.c_str() << "blocks" << makerCltvDeltaHours;

    if (!deal.makerCltvDelta) {
        failDeal(deal, SwapFailureReason::UnexpectedClientError,
            std::string("Could not calculate makerCltvDelta."));
        throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
    }

    std::string paymentRequest;
    if (makerSwapClient->addHodlInvoice(deal.rHash, deal.makerUnits, deal.makerCltvDelta.get())
            .tap([&paymentRequest](std::string req) { paymentRequest = req; })
            .wait()
            .isRejected()) {
        failDeal(deal, SwapFailureReason::UnexpectedClientError,
            std::string("could not add invoice for while accepting deal"));
        throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
    }

    // persist the swap deal to the database after we've added an invoice for it
    setDealPhase(deal, SwapPhase::SwapAccepted);

    packets::SwapAcceptedPacketBody responseBody;
    responseBody.set_rhash(body.rhash());
    responseBody.set_quantity(body.proposedquantity());
    responseBody.set_makercltvdelta(deal.makerCltvDelta.get_value_or(1));
    responseBody.set_makerdestination(makerDestination);
    responseBody.set_makerpaymentrequest(paymentRequest);

    LogCDebug(Swaps) << "sending swap accepted packet to peer:" << peer->nodePubKey().c_str()
                     << "rHash: " << body.rhash().c_str();

    setTimeout(deal.rHash, SWAP_INVOICE_EXCHANGE_TIMEOUT, SwapFailureReason::SwapTimedOut);
    QPointer<SwapManager> self(this);
    peer->sendPacket(SerializePacket(responseBody)).tapFail([self, deal]() mutable {
        if (self) {
            self->failDeal(deal, SwapFailureReason::RemoteError,
                std::string("Failed to send SwapAcceptedPacketBody packet"));
        }
    });

    return true;
}

//==============================================================================

void SwapManager::handleResolveRequest(ResolveRequest request)
{
    QVariantMap payload;
    SwapDeal deal;

    try {
        deal = dealByHash(request.rHash);
    } catch (SwapException& ex) {
        return;
    }

    if ((deal.role == SwapRole::Maker && deal.makerPubKey != request.responderIdentifier)
        || (deal.role == SwapRole::Taker && deal.takerPubKey != request.responderIdentifier)) {
        return;
    }

    payload["channelAddress"] = QString::fromStdString(request.channelAddress);
    payload["transferId"] = QString::fromStdString(request.transferId);

    onHtlcAccepted(request.rHash, request.amount,
        deal.role == SwapRole::Maker ? deal.makerCurrency : deal.takerCurrency, payload);
}

//==============================================================================

std::string SwapManager::resolveHash(std::string rHash, u256 units, std::string currency)
{
    if (_deals.count(rHash) == 0) {
        if (units == 1) {
            return resolveSanitySwap(rHash, units, currency);
        } else {
            throw SwapException(SwapFailureReason::UnknownError, "no payment hash found");
        }
    }

    auto deal = dealByHash(rHash);

    auto expectedAmount = deal.role == SwapRole::Maker ? deal.makerUnits : deal.takerUnits;

    LogCCInfo(Swaps) << "Resolving hash for deal:" << rHash.c_str()
                     << "amount:" << units.str().data() << currency.c_str();

    if (expectedAmount != units) {
        LogCCritical(Swaps) << "Invalid payment amount, expected:" << expectedAmount.str().data()
                            << "received:" << units.str().data() << "rHash: " << rHash.c_str();
        failDeal(
            deal, SwapFailureReason::SendPaymentFailure, std::string("invalid remote payment"));
        throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
    }

    if (deal.role == SwapRole::Maker) {
        // As the maker, we need to forward the payment to the other chain
        Q_ASSERT_X(currency.empty() || currency == deal.makerCurrency, __func__,
            "incoming htlc does not match expected deal currency");

        LogCDebug(Swaps) << "Executing maker code to resolve hash: " << rHash.c_str();

        auto swapClient = _clients.getClient(deal.takerCurrency);

        // we update the phase persist the deal to the database before we attempt to send payment
        setDealPhase(deal, SwapPhase::SendingPayment);

        std::string rPreimage;
        {
            std::string sendPaymentError;
            auto promise
                = swapClient->sendPayment(deal)
                      .tap([&rPreimage](std::string value) { rPreimage = value; })
                      .tapFail([&sendPaymentError](std::string error) { sendPaymentError = error; })
                      .wait();
            if (promise.isRejected()) {
                failDeal(deal, SwapFailureReason::SendPaymentFailure, sendPaymentError);
                throw SwapException(deal.failureReason.get(), deal.errorMessage.get());
            }
        }

        return rPreimage;
    } else {
        // If we are here we are the taker
        Q_ASSERT_X(deal.rPreimage, __func__, "preimage must be known if we are the taker");
        Q_ASSERT_X(currency.empty() || currency == deal.takerCurrency, __func__,
            "incoming htlc does not match expected deal currency");
        LogCDebug(Swaps) << "Executing taker code to resolve hash" << deal.rHash.c_str();

        return deal.rPreimage.get();
    }
}

//==============================================================================

std::string SwapManager::resolveSanitySwap(std::string rHash, u256 amount, std::string currency)
{
    return std::string();
}

//==============================================================================

void SwapManager::setTimeout(SwapManager::RHash rHash, int timeoutMs, SwapFailureReason reason)
{
    auto& timeoutData = _timeouts[rHash];
    timeoutData.failReason = reason;
    timeoutData.timerId = startTimer(timeoutMs);
}

//==============================================================================

void SwapManager::clearTimeout(SwapManager::RHash rHash)
{
    if (_timeouts.count(rHash) > 0) {
        killTimer(_timeouts.at(rHash).timerId);
        _timeouts.erase(rHash);
    }
}

//==============================================================================
}
