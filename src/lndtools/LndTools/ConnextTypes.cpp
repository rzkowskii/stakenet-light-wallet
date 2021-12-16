#include "ConnextTypes.hpp"
#include <QDebug>
#include <cmath>

//==============================================================================

QStringList ConnextChannel::EnumerateAssets(const QVariantMap& channel)
{
    QStringList result;
    auto tokenIds = channel.value("assetIds").toList();
    std::transform(std::begin(tokenIds), std::end(tokenIds), std::back_inserter(result),
        std::mem_fn(&QVariant::toString));
    return result;
}

//==============================================================================

ConnextChannel ConnextChannel::FromVariantChannelConnextChannel(
    const QVariantMap& channel, QString identifier, QString tokenAddress)
{
    ConnextChannel connextChannel;
    connextChannel.active = true;
    connextChannel.csvDelay = channel.value("timeout").toInt();
    connextChannel.contractAddress = tokenAddress;

    eth::u256 localBalance{ 0 }, remoteBalance{ 0 };
    auto tokenIds = channel.value("assetIds").toList();
    auto processedDepositsA = channel.value("processedDepositsA").toString();
    auto processedDepositsB = channel.value("processedDepositsB").toString();

    for (auto i = 0; i < tokenIds.size(); ++i) {
        if (tokenIds[i].toString() == tokenAddress) {

            // Connext returns same json for both channel roles, so here we should take correct
            // balances depending on public identifier
            if (channel.value("aliceIdentifier").toString() == identifier) {

                localBalance = eth::u256{ channel.value("balances")
                                              .value<QVariantList>()
                                              .at(i)
                                              .toMap()
                                              .value("amount")
                                              .value<QVariantList>()
                                              .at(0)
                                              .toString()
                                              .toStdString() };

                remoteBalance = eth::u256{ channel.value("balances")
                                               .value<QVariantList>()
                                               .at(i)
                                               .toMap()
                                               .value("amount")
                                               .value<QVariantList>()
                                               .at(1)
                                               .toString()
                                               .toStdString() };
            } else {

                localBalance = eth::u256{ channel.value("balances")
                                              .value<QVariantList>()
                                              .at(i)
                                              .toMap()
                                              .value("amount")
                                              .value<QVariantList>()
                                              .at(1)
                                              .toString()
                                              .toStdString() };

                remoteBalance = eth::u256{ channel.value("balances")
                                               .value<QVariantList>()
                                               .at(i)
                                               .toMap()
                                               .value("amount")
                                               .value<QVariantList>()
                                               .at(0)
                                               .toString()
                                               .toStdString() };
            }
        }
    }

    connextChannel.remotePubKey = channel.value("aliceIdentifier").toString() == identifier
        ? channel.value("bobIdentifier").toString()
        : channel.value("aliceIdentifier").toString();

    connextChannel.capacity = localBalance + remoteBalance;
    connextChannel.localBalance = localBalance;
    connextChannel.remoteBalance = remoteBalance;
    connextChannel.channelAddress = channel.value("channelAddress").toString();
    return connextChannel;
}

//==============================================================================

std::array<eth::u256, 2> BuildConnextChannelsBalance(
    const std::vector<ConnextChannel>& channels)
{
    std::array<eth::u256, 2> result{ 0, 0 };
    for (auto&& channel : channels) {
        result.at(0) += std::max<eth::u256>(channel.localBalance, 0);
        result.at(1) += std::max<eth::u256>(channel.remoteBalance, 0);
    }

    return result;
}

//==============================================================================
