#include "Packets.hpp"

namespace swaps {

packets::Packet UnserializePacket(std::vector<unsigned char> data)
{
    packets::Packet packet;
    packet.ParseFromArray(data.data(), data.size());
    return packet;
}

template <> SwapDeal ConstructWith(const packets::SwapRequestPacketBody& b)
{
    SwapDeal result;
    result.proposedQuantity = b.proposedquantity();
    result.pairId = b.pairid();
    result.orderId = b.orderid();
    result.rHash = b.rhash();
    result.takerCltvDelta = b.takercltvdelta();

    return result;
}

template <> void BuildPacket(packets::Packet& packet, packets::SwapRequestPacketBody* body) noexcept
{
    packet.set_allocated_req(body);
}

template <> void BuildPacket(packets::Packet& packet, packets::SwapFailedPacketBody* body) noexcept
{
    packet.set_allocated_fail(body);
}

template <>
void BuildPacket(packets::Packet& packet, packets::SwapAcceptedPacketBody* body) noexcept
{
    packet.set_allocated_accepted(body);
}

template <>
void BuildPacket(packets::Packet& packet, packets::SwapCompletePacketBody* body) noexcept
{
    packet.set_allocated_complete(body);
}

template <>
void BuildPacket(packets::Packet& packet, packets::InvoiceExchangePacketBody* body) noexcept
{
    packet.set_allocated_invoice_exchange(body);
}

template <>
void BuildPacket(packets::Packet& packet, packets::InvoiceExchangeAckPacketBody* body) noexcept
{
    packet.set_allocated_exchange_ack(body);
}
}
