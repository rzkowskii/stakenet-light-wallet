#ifndef CONNEXTCHANNEL_HPP
#define CONNEXTCHANNEL_HPP

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <array>

#include <EthCore/Types.hpp>

struct ConnextChannel {
    static QStringList EnumerateAssets(const QVariantMap& channel);
    static ConnextChannel FromVariantChannelConnextChannel(
        const QVariantMap& channel, QString identifier, QString tokenAddress);
    // { LocalBalance, RemoteBalance }

    bool active;
    QString contractAddress;
    QString remotePubKey;
    QString channelAddress;
    eth::u256 capacity;
    eth::u256 localBalance;
    eth::u256 remoteBalance;
    uint32_t csvDelay;
    QVariantMap details;

    bool operator==(const ConnextChannel& rhs) const
    {
        return channelAddress == rhs.channelAddress && localBalance == rhs.localBalance && remoteBalance == rhs.remoteBalance;
    }

    bool operator!=(const ConnextChannel& rhs) const { return !(*this == rhs); }

private:
    ConnextChannel() {}
};

std::array<eth::u256, 2> BuildConnextChannelsBalance(
    const std::vector<ConnextChannel>& channels);

#endif // CONNEXTCHANNEL_HPP
