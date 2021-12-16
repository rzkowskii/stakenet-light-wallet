#ifndef PACKETS_HPP
#define PACKETS_HPP

#include <Swaps/Protos/Packets.pb.h>
#include <Swaps/Types.hpp>

namespace swaps {

template <typename T> struct StringAnnotationTypeMap {
    static const std::string annotation;
};

template <class T> void BuildPacket(packets::Packet& packet, T* body) noexcept;

template <class T> packets::Packet SerializePacket(T body)
{
    packets::Packet packet;
    T* newBody = new T(body);
    BuildPacket(packet, newBody);
    return packet;
}

packets::Packet UnserializePacket(std::vector<unsigned char> data);

template <class R, class T> R ConstructWith(const T& value);
}
#endif // PACKETS_HPP
