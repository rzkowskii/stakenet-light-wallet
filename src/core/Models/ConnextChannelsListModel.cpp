#include "ConnextChannelsListModel.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Models/ChannelRentalHelper.hpp>
#include <Models/ConnextDaemonInterface.hpp>
#include <Tools/Common.hpp>

//==============================================================================

ConnextChannelsListModel::ConnextChannelsListModel(AssetID assetID,
    PaymentNodeInterface* paymentNodeInterface, QPointer<WalletAssetsModel> assetsModel,
    ChannelRentalHelper* channelRental, QObject* parent)
    : PaymentChannelsListModel(assetID, parent)
    , _paymentNode(qobject_cast<ConnextDaemonInterface*>(paymentNodeInterface))
    , _assetsModel(assetsModel)
    , _channelRentalHelper(channelRental)
{
    Q_ASSERT_X(_paymentNode, __FUNCTION__, "Expecting connext payment node");
    init();
}

//==============================================================================

void ConnextChannelsListModel::refresh()
{
    _paymentNode->refreshChannels();
}

//==============================================================================

int ConnextChannelsListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return _connextChannels.size();
}

//==============================================================================

QVariant ConnextChannelsListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }

    auto channel = _connextChannels.at(index.row());
    auto sourceDecimals = UNITS_PER_CURRENCY.at(_assetID);
    auto channelAddress = channel.channelAddress;
    auto isChannelRental = isRentalChannel(channelAddress);

    switch (role) {
    case TotalSatoshisRecvRole:
        return 0;
    case TotalSatoshisSentRole:
        return 0;
    case RemotePubKeyRole:
        return channel.remotePubKey;
    case LocalBalanceRole:
        return static_cast<double>(
            eth::ConvertFromWeiToSats(channel.localBalance, sourceDecimals, 8));
    case RemoteBalanceRole:
        return static_cast<double>(
            eth::ConvertFromWeiToSats(channel.remoteBalance, sourceDecimals, 8));
    case ChannelIdRole:
        return channelAddress;
    case ChannelPointRole:
        return "";
    case CapacityRole:
        return static_cast<double>(eth::ConvertFromWeiToSats(channel.capacity, sourceDecimals, 8));
    case TypeRole:
        return static_cast<int>(ChannelType::Active);
    case CSVDelayRole:
        return channel.csvDelay;
    case FundingTxIdRole:
        return QString();
    case ExpiresAtRole:
        return isChannelRental ? static_cast<double>(getExpiresAt(channelAddress)) : 0;
    case IsRentingChannelRole:
        return isChannelRental;
    case ChannelDateRole:
        return isChannelRental ? static_cast<double>(getRentalCreatedDate(channelAddress))
                               : 0; // TODO: uliana
    case DetailsRole:
        return channel.details;
    default:
        break;
    }

    return {};
}

//==============================================================================

QVariantMap ConnextChannelsListModel::get(QString channelAddress)
{
    QVariantMap result;

    int row = 0;
    auto channelIt = std::find_if(std::begin(_connextChannels), std::end(_connextChannels),
        [channelAddress](
            const ConnextChannel& channel) { return channel.channelAddress == channelAddress; });

    if (channelIt != std::end(_connextChannels)) {
        row = static_cast<int>(std::distance(std::begin(_connextChannels), channelIt));
        QHash<int, QByteArray> names = roleNames();
        QHashIterator<int, QByteArray> it(names);
        while (it.hasNext()) {
            it.next();
            QModelIndex idx = index(row);
            QVariant data = idx.data(it.key());
            result[it.value()] = data;
        }
    }
    return result;
}

//==============================================================================

QVariantMap ConnextChannelsListModel::getByIndex(int row)
{
    QHash<int, QByteArray> names = roleNames();
    QHashIterator<int, QByteArray> it(names);
    QVariantMap result;
    while (it.hasNext()) {
        it.next();
        QModelIndex idx = index(row);
        QVariant data = idx.data(it.key());
        result[it.value()] = data;
    }
    return result;
}

//==============================================================================

void ConnextChannelsListModel::onUpdateChannels(ConnextChannelsListModel::ConnextChannels channels)
{
    if (_connextChannels != channels) {
        beginResetModel();
        _connextChannels = channels;
        endResetModel();
    }
}

//==============================================================================

void ConnextChannelsListModel::init()
{
    connect(_paymentNode, &ConnextDaemonInterface::channelsChanged, this,
        &ConnextChannelsListModel::onUpdateChannels);
    QPointer<ConnextChannelsListModel> self{ this };
    _paymentNode->channels().then([self](ConnextChannels channels) {
        if (self) {
            self->onUpdateChannels(channels);
        }
    });
    if (_channelRentalHelper) {
        connect(_channelRentalHelper, &ChannelRentalHelper::channelAdded, this,
            &ConnextChannelsListModel::onUpdateRentalChannels);
        connect(_channelRentalHelper, &ChannelRentalHelper::channelChanged, this,
            &ConnextChannelsListModel::onUpdateRentalChannels);
    }
}

//==============================================================================

bool ConnextChannelsListModel::isRentalChannel(QString channelAddress) const
{
    return _channelRentalHelper->isRental(
        _assetID, orderbook::ChannelRentingManager::ChannelType::CONNEXT, channelAddress);
}

//==============================================================================

void ConnextChannelsListModel::onUpdateRentalChannels(AssetID assetID, QString channelAddress)
{
    if (_assetID == assetID) {
        dataChanged(
            index(0), index(rowCount(QModelIndex()) - 1), { ExpiresAtRole, IsRentingChannelRole });
    }
}

//==============================================================================

uint64_t ConnextChannelsListModel::getExpiresAt(QString channelAddress) const
{
    return _channelRentalHelper->expiresTime(
        _assetID, orderbook::ChannelRentingManager::ChannelType::CONNEXT, channelAddress);
}

//==============================================================================

uint64_t ConnextChannelsListModel::getRentalCreatedDate(QString channelAddress) const
{
    return _channelRentalHelper->createdTime(
        _assetID, orderbook::ChannelRentingManager::ChannelType::CONNEXT, channelAddress);
}

//==============================================================================
