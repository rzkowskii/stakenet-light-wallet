#include "OrderbookSwapPeerPool.hpp"
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/OrderbookEventDispatcher.hpp>
#include <Orderbook/OrderbookSwapPeer.hpp>
#include <Utils/Logging.hpp>

namespace orderbook {

//==============================================================================

OrderbookSwapPeerPool::OrderbookSwapPeerPool(OrderbookClient& client, QObject* parent)
    : swaps::AbstractSwapPeerPool(parent)
    , _client(client)
{
    _client.dispatcher().addEventHandler(Event::ServerEvent::ValueCase::kNewOrderMessage,
        std::bind(&OrderbookSwapPeerPool::onPeerMessageReceived, this, std::placeholders::_1));
}

//==============================================================================

OrderbookSwapPeerPool::~OrderbookSwapPeerPool() {}

//==============================================================================

swaps::AbstractSwapPeer* OrderbookSwapPeerPool::peerByPubKey(std::string pubKey) const
{
    return getPeerByPubKey(pubKey);
}

//==============================================================================

void OrderbookSwapPeerPool::addSwapPeer(std::string pubKey)
{
    if (getPeerByPubKey(pubKey)) {
        return;
    }

    LogCDebug(Orderbook) << "Adding swap peer" << pubKey.c_str();
    auto onSendPacket
        = [this, id = pubKey](auto serialized) { return _client.sendOrderMessage(id, serialized); };
    _connectedPeers.emplace(
        pubKey, std::unique_ptr<OrderbookSwapPeer>(new OrderbookSwapPeer(pubKey, onSendPacket)));
}

//==============================================================================

void OrderbookSwapPeerPool::onPeerMessageReceived(Event::ServerEvent event)
{
    auto message = event.newordermessage();
    auto id = message.orderid();
    if (auto peer = getPeerByPubKey(id)) {
        // in case of orderbook is sending messages, we will parse everything here and just
        // pass to the listening clients
        handlePeerMessage(peer, message.message());
    }
}

//==============================================================================

OrderbookSwapPeer* OrderbookSwapPeerPool::getPeerByPubKey(std::string pubKey) const
{
    return _connectedPeers.count(pubKey) > 0 ? _connectedPeers.at(pubKey).get() : nullptr;
}

//==============================================================================

void OrderbookSwapPeerPool::handlePeerMessage(OrderbookSwapPeer* peer, std::string serialized)
{
    packets::Packet packet;
    packet.ParseFromString(serialized);

    auto rHash = [packet] {
        switch (packet.swap_case()) {
        case packets::Packet::SwapCase::kReq:
            return packet.req().rhash();
        case packets::Packet::SwapCase::kAccepted:
            return packet.accepted().rhash();
        case packets::Packet::SwapCase::kComplete:
            return packet.complete().rhash();
        case packets::Packet::SwapCase::kFail:
            return packet.fail().rhash();
        case packets::Packet::SwapCase::kInvoiceExchange:
            return packet.invoice_exchange().rhash();
        case packets::Packet::SwapCase::kExchangeAck:
            return packet.exchange_ack().rhash();
        default:
            break;
        }

        return std::string{};
    }();

    if (_lastSwapState.count(rHash) > 0 && _lastSwapState.at(rHash) == packet.swap_case()) {
        return;
    }
    _lastSwapState[rHash] = packet.swap_case();

    switch (packet.swap_case()) {
    case packets::Packet::SwapCase::kReq:
        swapRequestReceived(packet.req(), peer);
        break;
    case packets::Packet::SwapCase::kAccepted:
        swapAcceptedReceived(packet.accepted(), peer);
        break;
    case packets::Packet::SwapCase::kComplete:
        swapCompleteReceived(packet.complete(), peer);
        break;
    case packets::Packet::SwapCase::kFail:
        swapFailReceived(packet.fail(), peer);
        break;
    case packets::Packet::SwapCase::kInvoiceExchange:
        swapInvoiceExchangeReceived(packet.invoice_exchange(), peer);
        break;
    case packets::Packet::SwapCase::kExchangeAck:
        swapInvoiceExchangeAckReceived(packet.exchange_ack(), peer);
        break;
    default:
        break;
    }
}

//==============================================================================
}
