#ifndef CHANNELRENTALHELPER_HPP
#define CHANNELRENTALHELPER_HPP

#include <Service/ChannelRentingManager.hpp>
#include <Tools/Common.hpp>

#include <QObject>
#include <unordered_map>

class DexService;
class WalletAssetsModel;

namespace orderbook {
class ChannelRentingManager;
}

class ChannelRentalHelper : public QObject {
    Q_OBJECT
public:
    explicit ChannelRentalHelper(
        DexService& dexService, WalletAssetsModel& assetsModel, QObject* parent = nullptr);
    virtual ~ChannelRentalHelper();

    void start();
    // channelIdentifier: LND - funding outpoint
    //                    CONNEXT - channel address
    bool isRental(AssetID assetID, storage::ChannelType type, QString channelIdentifier) const;
    bool canOpenRentalChannel(AssetID assetID) const;
    int64_t expiresTime(AssetID assetID, storage::ChannelType type, QString channelIdentifier) const;
    int64_t createdTime(AssetID assetID, storage::ChannelType type, QString channelIdentifier) const;

    QString channelIDByTxOutpoint(AssetID assetID, QString fundingOutpoint) const;

signals:
    void channelAdded(AssetID assetID, QString fundingOutpoints = QString());
    void channelChanged(AssetID assetID, QString fundingOutpoints = QString());

private slots:
    void onChannelsAdded(orderbook::ChannelRentingManager::RentedChannels details);
    void onChannelsChanged(orderbook::ChannelRentingManager::RentedChannels details);

private:
    void initChannels();

private:
    WalletAssetsModel& _assetsModel;
    DexService& _dexService;
    orderbook::ChannelRentingManager* _channelRentingManager{ nullptr };
    std::unordered_map<AssetID, orderbook::ChannelRentingManager::RentedChannels> _channels;
};

#endif // CHANNELRENTALHELPER_HPP
