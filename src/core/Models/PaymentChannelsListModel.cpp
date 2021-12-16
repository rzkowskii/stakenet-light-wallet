#include "PaymentChannelsListModel.hpp"

#include <Models/PaymentNodeInterface.hpp>

//==============================================================================

PaymentChannelsListModel::PaymentChannelsListModel(
    AssetID assetID, QObject* parent)
    : QAbstractListModel(parent)
    , _assetID(assetID)
{
}

//==============================================================================

int PaymentChannelsListModel::count() const
{
    return rowCount();
}

//==============================================================================

QHash<int, QByteArray> PaymentChannelsListModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[TotalSatoshisRecvRole] = "totalReceive";
        roles[TotalSatoshisSentRole] = "totalSent";
        roles[RemotePubKeyRole] = "remotePubKey";
        roles[LocalBalanceRole] = "localBalance";
        roles[RemoteBalanceRole] = "remoteBalance";
        roles[ChannelIdRole] = "channelID";
        roles[ChannelPointRole] = "channelOutpoint";
        roles[TypeRole] = "type";
        roles[CapacityRole] = "capacity";
        roles[CSVDelayRole] = "csvDelay";
        roles[DetailsRole] = "details";
        roles[FundingTxIdRole] = "fundingTxId";
        roles[ConfirmationsRole] = "confirmations";
        roles[ExpiresAtRole] = "expiresAtTime";
        roles[IsRentingChannelRole] = "isRentingChannel";
        roles[ChannelDateRole] = "channelDate";
    }

    return roles;
}

//==============================================================================
