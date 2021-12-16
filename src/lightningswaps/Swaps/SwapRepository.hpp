#ifndef SWAPREPOSITORY_HPP
#define SWAPREPOSITORY_HPP

#include <QObject>
#include <Swaps/AbstractSwapRepository.hpp>
#include <memory>
#include <serialize.h>

namespace bitcoin {

template <typename Stream, typename E> inline void SerializeEnum(Stream& s, E a)
{
    ser_writedata32(s, static_cast<int32_t>(a));
}
template <typename Stream, typename E> inline void UnserializeEnum(Stream& s, E& a)
{
    a = static_cast<E>(ser_readdata32(s));
}

template <typename Stream> inline void Serialize(Stream& s, swaps::SwapRole a)
{
    SerializeEnum(s, a);
}
template <typename Stream> inline void Unserialize(Stream& s, swaps::SwapRole& a)
{
    UnserializeEnum(s, a);
}
template <typename Stream> inline void Serialize(Stream& s, swaps::SwapPhase a)
{
    SerializeEnum(s, a);
}
template <typename Stream> inline void Unserialize(Stream& s, swaps::SwapPhase& a)
{
    UnserializeEnum(s, a);
}
template <typename Stream> inline void Serialize(Stream& s, swaps::SwapState a)
{
    SerializeEnum(s, a);
}
template <typename Stream> inline void Unserialize(Stream& s, swaps::SwapState& a)
{
    UnserializeEnum(s, a);
}
template <typename Stream> inline void Serialize(Stream& s, swaps::SwapFailureReason a)
{
    SerializeEnum(s, a);
}
template <typename Stream> inline void Unserialize(Stream& s, swaps::SwapFailureReason& a)
{
    UnserializeEnum(s, a);
}

template <typename Stream, typename T> void Serialize(Stream& os, const boost::optional<T>& p)
{
    if (p) {
        WriteCompactSize(os, 1);
        Serialize(os, p.value());
    } else {
        WriteCompactSize(os, 0);
    }
}
template <typename Stream, typename T> void Unserialize(Stream& os, boost::optional<T>& p)
{
    if (auto hasValue = ReadCompactSize(os) > 0) {
        T value;
        Unserialize(os, value);
        p = value;
    }
}

template <typename Stream> void Serialize(Stream& os, const swaps::u256& value)
{
    std::vector<unsigned char> v;
    boost::multiprecision::export_bits(value, std::back_inserter(v), 8);
    Serialize(os, v);
}
template <typename Stream> void Unserialize(Stream& os, swaps::u256& value)
{
    std::vector<unsigned char> v;
    Unserialize(os, v);
    boost::multiprecision::import_bits(value, v.begin(), v.end(), 8);
}

template <typename Stream> void Serialize(Stream& os, const swaps::SwapDeal& deal)
{
    os << deal.role << deal.phase << deal.state << deal.errorMessage << deal.failureReason
       << deal.peerPubKey << deal.orderId << deal.isBuy << deal.localId << deal.proposedQuantity
       << deal.quantity << deal.pairId << deal.takerPubKey << deal.takerAmount << deal.takerUnits
       << deal.takerCurrency << deal.takerCltvDelta << deal.makerAmount << deal.makerUnits
       << deal.makerCurrency << deal.makerCltvDelta << deal.price << deal.rHash << deal.rPreimage
       << deal.takerMaxTimeLock << deal.destination << deal.createTime << deal.executeTime
       << deal.completeTime;
}

template <typename Stream> void Unserialize(Stream& os, swaps::SwapDeal& deal)
{
    os >> deal.role >> deal.phase >> deal.state >> deal.errorMessage >> deal.failureReason
        >> deal.peerPubKey >> deal.orderId >> deal.isBuy >> deal.localId >> deal.proposedQuantity
        >> deal.quantity >> deal.pairId >> deal.takerPubKey >> deal.takerAmount >> deal.takerUnits
        >> deal.takerCurrency >> deal.takerCltvDelta >> deal.makerAmount >> deal.makerUnits
        >> deal.makerCurrency >> deal.makerCltvDelta >> deal.price >> deal.rHash >> deal.rPreimage
        >> deal.takerMaxTimeLock >> deal.destination >> deal.createTime >> deal.executeTime
        >> deal.completeTime;
}
}

namespace swaps {

class SwapRepository : public AbstractSwapRepository {
    Q_OBJECT
public:
    explicit SwapRepository(std::string dataDir, QObject* parent = nullptr);
    ~SwapRepository() override;

    // AbstractSwapRepository interface
protected:
    Deals executeLoadAll() override;
    void executeSaveDeal(SwapDeal deal) override;

private:
    struct DBImpl;
    std::unique_ptr<DBImpl> _db;
};
}

#endif // SWAPREPOSITORY_HPP
