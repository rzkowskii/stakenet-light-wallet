#ifndef ABSTRACTPEERPOOL_HPP
#define ABSTRACTPEERPOOL_HPP

#include <QObject>
#include <Swaps/Packets.hpp>

namespace swaps {

class AbstractSwapPeer;

class AbstractSwapPeerPool : public QObject {
    Q_OBJECT
public:
    explicit AbstractSwapPeerPool(QObject* parent = nullptr);
    virtual AbstractSwapPeer* peerByPubKey(std::string pubKey) const = 0;

signals:
    void swapRequestReceived(
        packets::SwapRequestPacketBody packet, QPointer<AbstractSwapPeer> peer);
    void swapAcceptedReceived(
        packets::SwapAcceptedPacketBody packet, QPointer<AbstractSwapPeer> peer);
    void swapInvoiceExchangeReceived(
        packets::InvoiceExchangePacketBody packet, QPointer<AbstractSwapPeer> peer);
    void swapInvoiceExchangeAckReceived(
        packets::InvoiceExchangeAckPacketBody packet, QPointer<AbstractSwapPeer> peer);
    void swapCompleteReceived(
        packets::SwapCompletePacketBody packet, QPointer<AbstractSwapPeer> peer);
    void swapFailReceived(packets::SwapFailedPacketBody packet, QPointer<AbstractSwapPeer> peer);

public slots:
};
}

#endif // ABSTRACTPEERPOOL_HPP
