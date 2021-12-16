#ifndef ABSTRACTSWAPPEER_HPP
#define ABSTRACTSWAPPEER_HPP

#include <QObject>
#include <Swaps/Types.hpp>
#include <Swaps/Protos/Packets.pb.h>
#include <Utils/Utils.hpp>

namespace swaps {

class AbstractSwapPeer : public QObject {
    Q_OBJECT
public:
    explicit AbstractSwapPeer(std::string nodePubKey, QObject* parent = nullptr);

    virtual Promise<void> sendPacket(packets::Packet packet) = 0;

    std::string nodePubKey() const;

signals:
    void packetReceived(std::vector<unsigned char> serialized);

public slots:

private:
    std::string _nodePubKey;
};
}

#endif // ABSTRACTSWAPPEER_HPP
