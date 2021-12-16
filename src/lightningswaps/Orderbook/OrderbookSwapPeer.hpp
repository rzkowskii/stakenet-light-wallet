#ifndef ORDERBOOKSWAPPEER_HPP
#define ORDERBOOKSWAPPEER_HPP

#include <QObject>

#include <Swaps/AbstractSwapPeer.hpp>
#include <Swaps/Protos/Packets.pb.h>

namespace orderbook {

using SendPacket = std::function<Promise<void>(std::vector<unsigned char>)>;

class OrderbookSwapPeer : public swaps::AbstractSwapPeer {
    Q_OBJECT
public:
    explicit OrderbookSwapPeer(
        std::string orderId, SendPacket onSendPacket, QObject* parent = nullptr);
    ~OrderbookSwapPeer() override;

    Promise<void> sendPacket(packets::Packet packet) override;

public slots:

private:
    SendPacket _onSendPacket;
};
}

#endif // ORDERBOOKSWAPPEER_HPP
