#ifndef ABSTRACTSWAPSMANAGER_HPP
#define ABSTRACTSWAPSMANAGER_HPP

#include <QObject>
#include <Swaps/Packets.hpp>
#include <Swaps/Types.hpp>
#include <Utils/Utils.hpp>
#include <unordered_map>
#include <unordered_set>

namespace orderbook {
class AbstractOrderbookClient;
};

namespace swaps {

class AbstractSwapPeer;
class AbstractSwapPeerPool;
class AbstractSwapClientPool;
class AbstractSwapRepository;

/*!
 * \brief The SwapManager class contains all logic to execute a swap. It's intended to be used
 * as interface to swaping code.
 * TODO(yuraolex): This class misses any handling related to timeouts. Need to add proper error
 * handling in case of other side is not responding
 */
class SwapManager : public QObject {
    Q_OBJECT
public:
    using RHash = std::string;

    explicit SwapManager(const AbstractSwapPeerPool& peers, AbstractSwapClientPool& clients,
        AbstractSwapRepository& repository, orderbook::AbstractOrderbookClient& orderbook,
        QObject* parent = nullptr);

    Promise<SwapSuccess> executeSwap(PeerOrder maker, OwnOrder taker);

signals:
    void swapPaid(SwapSuccess success);
    void swapFailed(SwapDeal deal);

    // QObject interface
protected:
    void timerEvent(QTimerEvent* event) override;

private slots:
    /*!
     * \brief onSwapRequested Slot that is called on maker side when swap was taken by taker
     * \param body swap details
     * \param peer taker of swap order, used to send response
     */
    void onSwapRequested(packets::SwapRequestPacketBody body, QPointer<AbstractSwapPeer> peer);
    /*!
     * \brief onSwapAccepted Slot that is called as repsonse to \ref acceptDeal, called on taker
     * side \param body deal details that were accepted by maker \param peer maker of swap order,
     * used to send response
     */
    void onSwapAccepted(packets::SwapAcceptedPacketBody body, QPointer<AbstractSwapPeer> peer);
    void onInvoiceExchange(
        packets::InvoiceExchangePacketBody body, QPointer<AbstractSwapPeer> peer);
    void onInvoiceExchangeAck(
        packets::InvoiceExchangeAckPacketBody body, QPointer<AbstractSwapPeer> peer);
    /*!
     * \brief onSwapCompleted Slot that is called as response to [[SwapCompleted]] state from taker,
     * called on maker side \param body details of succseful deal \param peer taker of swap order
     */
    void onSwapCompleted(packets::SwapCompletePacketBody body, QPointer<AbstractSwapPeer> peer);
    void onSwapFailed(packets::SwapFailedPacketBody body, QPointer<AbstractSwapPeer> peer);
    /*!
     * \brief onHtlcAccepted Slot that is called from lnd to resolve pending HTLC,
     * means that payment was made and we need to resolve the rHash. Called on maker and taker side.
     * \param rHash hash of secret that needs to be resolved
     * \param amount of pending HTLC
     * \param currency of pending HTLC
     */
    void onHtlcAccepted(std::string rHash, int64_t amount, std::string currency, QVariantMap payload);

private:
    /*!
     * \brief beginSwap Begins a swap to fill an order by sending a [[SwapRequestPacket]] to the
     * maker. \param maker The remote maker order we are filling \param taker Our local taker order
     * \return The rHash for the swap, or throws a [[SwapException]] if the swap could not be
     * initiated
     */
    std::string beginSwap(PeerOrder maker, OwnOrder taker);
    void verifyExecution(PeerOrder maker, OwnOrder taker);
    AbstractSwapPeer& peerByPubKey(std::string pubKey) const;
    void setDealPhase(SwapDeal deal, SwapPhase newPhase);
    void persistDeal(SwapDeal deal);
    void addDeal(SwapDeal deal);
    void removeOrderHold(std::string orderId, std::string pairId, int64_t quantity);
    SwapDeal& dealByHash(std::string rHash);
    bool isPairSupported(std::string pairId) const;
    void connectSignals();
    void failDeal(
        SwapDeal& deal, SwapFailureReason reason, boost::optional<std::string> errorMessage = {});

    /*!
     * \brief acceptDeal Called from \ref onSwapRequested, called by maker to accept proposed deal
     * \param orderToAccept maker own order
     * \param body proposed order from taker
     * \param peer taker of swap order, used to send response
     * \return
     */
    bool acceptDeal(OrderToAccept orderToAccept, packets::SwapRequestPacketBody body,
        QPointer<AbstractSwapPeer> peer);

    void handleResolveRequest(ResolveRequest request);

    std::string resolveHash(std::string rHash, u256 units, std::string currency);
    std::string resolveSanitySwap(std::string rHash, u256 amount, std::string currency);
    void setTimeout(RHash rHash, int timeoutMs, SwapFailureReason reason);
    void clearTimeout(RHash rHash);

private:
    struct SwapTimeout {
        SwapFailureReason failReason;
        int timerId;
    };

    const AbstractSwapPeerPool& _peers;
    AbstractSwapClientPool& _clients;
    AbstractSwapRepository& _repository;
    orderbook::AbstractOrderbookClient& _orderbook;
    std::unordered_map<RHash, SwapDeal> _deals;
    std::unordered_set<RHash> _usedHashes;
    std::unordered_map<RHash, SwapTimeout> _timeouts;
};
}

#endif // ABSTRACTSWAPSMANAGER_HPP
