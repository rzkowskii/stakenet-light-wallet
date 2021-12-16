#ifndef LNDCHANNEL_HPP
#define LNDCHANNEL_HPP

#include <LndTools/Protos/LndTypes.pb.h>

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVariantMap>
#include <array>

static std::map<std::string, double> MINUTES_PER_BLOCK_BY_CURRENCY{ { "BTC", 10 }, { "LTC", 2.5 },
    { "XSN", 1 } };

static constexpr int LOCK_BUFFER_HOURS = 24;
const unsigned CHANNEL_RESERVE = 5000;
const unsigned MIN_SHARD_RESERVE = 200;
const unsigned HTLC_FEE_RATE_PARTS = 1000000;
const unsigned WITNESS_SCALE_FACTOR = 4;
const unsigned HTLC_SIZE = 43;
static const double LOCAL_CHAN_RESERVE_SAT = 0.01;
static const uint32_t MIN_FEE_PER_KW = 253;
static const uint32_t COMMIT_WEIGHT = 724;

namespace lnrpc {
class Channel;
class PendingChannelsResponse_PendingChannel;
enum PaymentFailureReason : int;
}

struct DaemonConfig {
    QString assetSymbol;
    QString rpcListenHost;
    QString listenHost;
    QString restListenHost;
    QString chain;
    QString tlsCert;
    QString tlsKey;
    QString macaroonPath;
    int zmqPort{ -1 };
    int cltvExpiry{ -1 };
    QString maxChanSizeSat;
    QStringList hostList;
    QStringList watchTowerList;
    double autopilotAllocation;
    unsigned autopilotMaxChannels;
    unsigned autopilotFeeRate;

    DaemonConfig(QString assetSym, QString rpcListen, QString listen, QString restListen,
        QString chain, QString tlsCert, QString tlsKey, QString macaroonPath, int zmqPort,
        int cltvExpiry, QString maxChanSizeSat, QStringList hostList, QStringList watchTowerList,
        double allocation, unsigned maxChannels, unsigned feeRate)
        : assetSymbol(assetSym)
        , rpcListenHost(rpcListen)
        , listenHost(listen)
        , restListenHost(restListen)
        , chain(chain)
        , tlsCert(tlsCert)
        , tlsKey(tlsKey)
        , macaroonPath(macaroonPath)
        , zmqPort(zmqPort)
        , cltvExpiry(cltvExpiry)
        , maxChanSizeSat(maxChanSizeSat)
        , hostList(hostList)
        , watchTowerList(watchTowerList)
        , autopilotAllocation(allocation)
        , autopilotMaxChannels(maxChannels)
        , autopilotFeeRate(feeRate)
    {
    }
};

struct ConnextDaemonConfig {
    QString host;
    int port{ 0 };
    QString hubHost;

    ConnextDaemonConfig() {}

    ConnextDaemonConfig(QString hostConf, int portConf, QString hubHost)
        : host(hostConf)
        , port(portConf)
        , hubHost(hubHost)
    {
    }
};

struct AssetLndConfig {
    unsigned confirmationForChannelApproved;

    explicit AssetLndConfig(unsigned channelConfirmations)
        : confirmationForChannelApproved(channelConfirmations)
    {
    }
};

struct LndChannel {
    static LndChannel FromRpcChannelLndChannel(const lnrpc::Channel& channel);
    static LndChannel FromRpcChannelPendingChannel(
        const lnrpc::PendingChannelsResponse_PendingChannel& channel);

    bool active;
    uint64_t channelId;
    QString remotePubKey;
    QString channelOutpoint;
    int64_t capacity;
    int64_t localBalance;
    int64_t remoteBalance;
    int64_t totalSatoshisSent;
    int64_t totalSatoshisRecv;
    uint32_t csvDelay;
    QVariantMap details;

    bool operator==(const LndChannel& rhs) const { return channelOutpoint == rhs.channelOutpoint; }

    bool operator!=(const LndChannel& rhs) const { return !(*this == rhs); }

private:
    LndChannel() {}
};

struct LightningPayRequest {
    Q_GADGET
    Q_PROPERTY(QString destination MEMBER destination CONSTANT)
    Q_PROPERTY(QString paymentHash MEMBER paymentHash CONSTANT)
    Q_PROPERTY(qint64 numSatoshis MEMBER numSatoshis CONSTANT)

public:
    QString destination;
    QString paymentHash;
    qint64 numSatoshis;
};
Q_DECLARE_METATYPE(LightningPayRequest)

bool ChannelHasEnoughBalance(int64_t totalBalances, int64_t amount);

// { LocalBalance, RemoteBalance }
std::array<int64_t, 2> BuildChannelsBalance(const std::vector<LndChannel>& channels);

int64_t MaxChannelBalance(std::vector<int64_t> balances);
int64_t CalculateBalance(std::vector<int64_t> balances);
int64_t BuildChannelReserve(int64_t amount, int64_t feeRate);

std::string PaymentFailureMsg(const lnrpc::PaymentFailureReason& paymentFailureReason);

#endif // LNDCHANNEL_HPP
