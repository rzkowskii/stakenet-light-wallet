#ifndef PAYMENTCHANNELSLISTMODEL_HPP
#define PAYMENTCHANNELSLISTMODEL_HPP

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>

#include <Tools/Common.hpp>

class PaymentNodeInterface;

class PaymentChannelsListModel : public QAbstractListModel {
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Roles {
        TotalSatoshisRecvRole = Qt::UserRole + 1,
        TotalSatoshisSentRole,
        RemotePubKeyRole,
        LocalBalanceRole,
        RemoteBalanceRole,
        ChannelPointRole,
        ChannelIdRole,
        CapacityRole,
        CanSend,
        CanReceive,
        TypeRole,
        CSVDelayRole,
        DetailsRole,
        FundingTxIdRole,
        ConfirmationsRole,
        ExpiresAtRole,
        IsRentingChannelRole,
        ChannelDateRole
    };

    enum class ChannelType { Active, Inactive, Pending, Closing, ForceClosing, Undefined };
    Q_ENUM(ChannelType)

    explicit PaymentChannelsListModel(AssetID assetID, QObject* parent = nullptr);

    int count() const;
    QHash<int, QByteArray> roleNames() const override;

public slots:
    virtual void refresh() = 0;

signals:
    void countChanged();

protected:
    AssetID _assetID;
};

#endif // PAYMENTCHANNELSLISTMODEL_HPP
