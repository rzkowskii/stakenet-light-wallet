#include "LndTypes.hpp"
#include <LndTools/Protos/rpc.grpc.pb.h>
#include <QJsonObject>
#include <cmath>

//==============================================================================

LndChannel LndChannel::FromRpcChannelLndChannel(const lnrpc::Channel& channel)
{
    LndChannel lndChannel;
    lndChannel.active = channel.active();
    lndChannel.remotePubKey = QString::fromStdString(channel.remote_pubkey());
    lndChannel.channelId = channel.chan_id();
    lndChannel.capacity = channel.capacity();
    lndChannel.localBalance = channel.local_balance();
    lndChannel.remoteBalance = channel.remote_balance();
    lndChannel.totalSatoshisSent = channel.total_satoshis_sent();
    lndChannel.totalSatoshisRecv = channel.total_satoshis_received();
    lndChannel.channelOutpoint = QString::fromStdString(channel.channel_point());
    lndChannel.csvDelay = channel.csv_delay();

    return lndChannel;
}

//==============================================================================

LndChannel LndChannel::FromRpcChannelPendingChannel(
    const lnrpc::PendingChannelsResponse_PendingChannel& channel)
{
    LndChannel pendingChannel;
    pendingChannel.remotePubKey = QString::fromStdString(channel.remote_node_pub());
    pendingChannel.capacity = channel.capacity();
    pendingChannel.localBalance = channel.local_balance();
    pendingChannel.remoteBalance = channel.remote_balance();
    pendingChannel.totalSatoshisSent = channel.local_chan_reserve_sat();
    pendingChannel.totalSatoshisRecv = channel.remote_chan_reserve_sat();
    pendingChannel.channelOutpoint = QString::fromStdString(channel.channel_point());

    return pendingChannel;
}

//==============================================================================

std::array<int64_t, 2> BuildChannelsBalance(const std::vector<LndChannel>& channels)
{
    std::array<int64_t, 2> result{ 0, 0 };
    for (auto&& channel : channels) {
        if (channel.active) {
            auto sendRecvCap
                = static_cast<int64_t>(channel.details.value("local_chan_reserve_sat").toDouble())
                + CHANNEL_RESERVE;
            auto sendHtlcFee = channel.localBalance / HTLC_FEE_RATE_PARTS;
            auto recvHtlcFee = channel.remoteBalance / HTLC_FEE_RATE_PARTS;

            auto isInitiator = channel.details.value("initiator").toBool();
            auto commitWeight = static_cast<int64_t>(channel.details.value("commit_weight").toDouble());
            auto feePerKw = static_cast<int64_t>(channel.details.value("fee_per_kw").toDouble());

            int64_t commitFee = 0;
            auto htlcWeight = WITNESS_SCALE_FACTOR * HTLC_SIZE;

            commitFee = feePerKw * (commitWeight + htlcWeight) / 1000;

            result.at(0) += std::max<int64_t>(channel.localBalance - sendRecvCap - sendHtlcFee - MIN_SHARD_RESERVE - commitFee, 0);
            result.at(1) += std::max<int64_t>(channel.remoteBalance - sendRecvCap - recvHtlcFee - MIN_SHARD_RESERVE - commitFee, 0);
        }
    }

    return result;
}

//==============================================================================

int64_t MaxChannelBalance(std::vector<int64_t> balances)
{
    auto it = std::max_element(balances.begin(), balances.end());
    if (it != balances.end()) {
        return *it;
    }
    return 0;
}

//==============================================================================

int64_t CalculateBalance(std::vector<int64_t> balances)
{
    return std::accumulate(balances.begin(), balances.end(), 0ll,
        [](qint64 value, const auto& balance) { return value + balance; });
}

//==============================================================================

std::string PaymentFailureMsg(const lnrpc::PaymentFailureReason& paymentFailureReason)
{
    using lnrpc::PaymentFailureReason;
    switch (paymentFailureReason) {
    case (PaymentFailureReason::FAILURE_REASON_NONE):
        return "The send payment has no error.";
    case (PaymentFailureReason::FAILURE_REASON_ERROR):
        return "Send payment failed due to an unrecognized error.";
    case (PaymentFailureReason::FAILURE_REASON_TIMEOUT):
        return "Send payment failed due to timeout.";
    case (PaymentFailureReason::FAILURE_REASON_NO_ROUTE):
        return "The LN network graph is waiting for updates, please wait for it to complete before "
               "placing an order. This may take up to 15 minutes.";
    case (PaymentFailureReason::FAILURE_REASON_INSUFFICIENT_BALANCE):
        return "Insufficient balance to send payment.";
    case (PaymentFailureReason::FAILURE_REASON_INCORRECT_PAYMENT_DETAILS):
        return "Incorrect payment details.";

    default:
        return { "Unknown reason" };
    }
}

//==============================================================================

bool ChannelHasEnoughBalance(int64_t totalBalances, int64_t amount)
{
    return totalBalances >= amount;
}

//==============================================================================

int64_t BuildChannelReserve(int64_t amount, int64_t feeRate)
{
    auto cap = static_cast<int64_t>(amount * LOCAL_CHAN_RESERVE_SAT) + CHANNEL_RESERVE;
    auto htlcFee = amount / HTLC_FEE_RATE_PARTS;

    auto feePerKw = static_cast<int64_t>(feeRate * 1000 / 4);
    if(feePerKw < MIN_FEE_PER_KW){
        feePerKw = MIN_FEE_PER_KW;
    }

    auto htlcWeight = WITNESS_SCALE_FACTOR * HTLC_SIZE;
    int64_t commitFee = feePerKw * (COMMIT_WEIGHT + htlcWeight) / 1000;

    return std::max<int64_t>(2 * (cap + htlcFee + MIN_SHARD_RESERVE + commitFee), 0);
}

//==============================================================================
