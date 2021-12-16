#include "ChannelRentalHelper.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Models/DexService.hpp>
#include <Service/ChannelRentingManager.hpp>

//==============================================================================

static storage::RentedChannel FindChannelByChannelIdentifier(
    const orderbook::ChannelRentingManager::RentedChannels& where, storage::ChannelType type,
    QString channelIdentifier)
{
    if (type == storage::ChannelType::LND) {
        auto it
            = std::find_if(where.begin(), where.end(), [channelIdentifier](const auto& channel) {
                  return channel.has_lnddetails()
                      && channel.lnddetails().fundingoutpoint() == channelIdentifier.toStdString();
              });

        return it != std::end(where) ? *it : storage::RentedChannel();
    } else {
        auto it = std::find_if(
            where.begin(), where.end(), [channelIdentifier](const auto& channel) {
                return channel.has_connextdetails()
                    && channel.connextdetails().channeladdress() == channelIdentifier.toStdString();
            });
        return it != std::end(where) ? *it : storage::RentedChannel();
    }
}

//==============================================================================

ChannelRentalHelper::ChannelRentalHelper(
    DexService& dexService, WalletAssetsModel& assetsModel, QObject* parent)
    : QObject(parent)
    , _assetsModel(assetsModel)
    , _dexService(dexService)
{
}

//==============================================================================

ChannelRentalHelper::~ChannelRentalHelper() {}

//==============================================================================

void ChannelRentalHelper::start()
{
    _channelRentingManager = _dexService.channelRentingManager();
    connect(_channelRentingManager, &orderbook::ChannelRentingManager::channelAdded, this,
        &ChannelRentalHelper::onChannelsAdded);
    connect(_channelRentingManager, &orderbook::ChannelRentingManager::channelChanged, this,
        &ChannelRentalHelper::onChannelsChanged);
    initChannels();
}

//==============================================================================

bool ChannelRentalHelper::isRental(
    AssetID assetID, storage::ChannelType type, QString channelIdentifier) const
{
    if (_channels.count(assetID) > 0) {
        auto& channels = _channels.at(assetID);
        auto channel = FindChannelByChannelIdentifier(channels, type, channelIdentifier);
        return !channel.channelid().empty();
    }
    return false;
}

//==============================================================================

bool ChannelRentalHelper::canOpenRentalChannel(AssetID assetID) const
{
    return _channels.count(assetID)
        < _assetsModel.assetById(assetID).lndData().rentalChannelsPerCoin;
}

//==============================================================================

int64_t ChannelRentalHelper::expiresTime(
    AssetID assetID, storage::ChannelType type, QString channelIdentifier) const
{
    if (_channels.count(assetID) > 0) {
        auto& channels = _channels.at(assetID);
        auto channel = FindChannelByChannelIdentifier(channels, type, channelIdentifier);
        if (!channel.channelid().empty()) {
            return channel.expiresat();
        }
    }
    return 0;
}

//==============================================================================

int64_t ChannelRentalHelper::createdTime(
    AssetID assetID, storage::ChannelType type, QString channelIdentifier) const
{
    if (_channels.count(assetID) > 0) {
        auto& channels = _channels.at(assetID);
        auto channel = FindChannelByChannelIdentifier(
            channels, storage::ChannelType::LND, channelIdentifier);
        if (!channel.channelid().empty()) {
            return channel.rentingdate();
        }
    }
    return 0;
}

//==============================================================================

QString ChannelRentalHelper::channelIDByTxOutpoint(AssetID assetID, QString fundingOutpoint) const
{
    if (_channels.count(assetID) > 0) {
        auto& channels = _channels.at(assetID);
        auto channel
            = FindChannelByChannelIdentifier(channels, storage::ChannelType::LND, fundingOutpoint);
        if (!channel.channelid().empty()) {
            return QString::fromStdString(channel.channelid());
        }
    }
    return QString();
}

//==============================================================================

void ChannelRentalHelper::onChannelsAdded(orderbook::ChannelRentingManager::RentedChannels details)
{
    for (auto channel : details) {
        auto assetID
            = _assetsModel.assetByName(QString::fromStdString(channel.currency())).coinID();
        _channels[assetID].push_back(channel);
        channelAdded(assetID,
            channel.type() == storage::ChannelType::LND
                ? QString::fromStdString(channel.lnddetails().fundingoutpoint())
                : QString::fromStdString(channel.connextdetails().channeladdress()));
    }
}

//==============================================================================

void ChannelRentalHelper::onChannelsChanged(
    orderbook::ChannelRentingManager::RentedChannels details)
{
    for (auto channel : details) {
        auto assetID
            = _assetsModel.assetByName(QString::fromStdString(channel.currency())).coinID();
        auto& assetChannels = _channels.at(assetID);

        if (channel.type() == storage::ChannelType::LND) {
            auto it = std::find_if(
                assetChannels.begin(), assetChannels.end(), [channel](const auto& chl) {
                    return chl.lnddetails().fundingoutpoint()
                        == channel.lnddetails().fundingoutpoint();
                });

            if (it != assetChannels.end()) {
                *it = channel;
                channelChanged(
                    assetID, QString::fromStdString(channel.lnddetails().fundingoutpoint()));
            }
        } else {
            channelChanged(
                assetID, QString::fromStdString(channel.connextdetails().channeladdress()));
        }
    }
}

//==============================================================================

void ChannelRentalHelper::initChannels()
{
    _channelRentingManager->channels().then(
        [this](orderbook::ChannelRentingManager::RentedChannels channels) {
            for (auto channel : channels) {
                auto assetID
                    = _assetsModel.assetByName(QString::fromStdString(channel.currency())).coinID();
                _channels[assetID].push_back(channel);
            }
        });
}

//==============================================================================
