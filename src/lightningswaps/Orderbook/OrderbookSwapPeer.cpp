#include "OrderbookSwapPeer.hpp"
#include <SwapService.hpp>
#include <Swaps/Packets.hpp>
#include <Utils/Logging.hpp>

namespace orderbook {

//==============================================================================

OrderbookSwapPeer::OrderbookSwapPeer(std::string orderId, SendPacket onSendPacket, QObject* parent)
    : swaps::AbstractSwapPeer(orderId, parent)
    , _onSendPacket(onSendPacket)
{
}

//==============================================================================

OrderbookSwapPeer::~OrderbookSwapPeer() {}

//==============================================================================

Promise<void> OrderbookSwapPeer::sendPacket(packets::Packet packet)
{
//    LogCDebug(Orderbook) << "sending packet:" << packet.DebugString().data();
    std::vector<unsigned char> blob(static_cast<size_t>(packet.ByteSize()));
    packet.SerializeToArray(blob.data(), blob.size());
    return _onSendPacket(blob);
}

//==============================================================================
}
