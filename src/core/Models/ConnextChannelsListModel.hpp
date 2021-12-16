#ifndef CONNEXTCHANNELSLISTMODEL_HPP
#define CONNEXTCHANNELSLISTMODEL_HPP

#include <QObject>

#include <LndTools/ConnextTypes.hpp>
#include <Models/PaymentChannelsListModel.hpp>

class ConnextDaemonInterface;
class WalletAssetsModel;
class ChannelRentalHelper;

class ConnextChannelsListModel : public PaymentChannelsListModel {
    Q_OBJECT
public:
    using ConnextChannels = std::vector<ConnextChannel>;
    explicit ConnextChannelsListModel(AssetID assetID, PaymentNodeInterface* paymentNodeInterface,
        QPointer<WalletAssetsModel> assetsModel, ChannelRentalHelper* channelRental,
        QObject* parent = nullptr);
    void refresh() override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

signals:

public slots:
    QVariantMap get(QString channelAddress);
    QVariantMap getByIndex(int row);

private slots:
    void onUpdateChannels(ConnextChannels channels);
    void onUpdateRentalChannels(AssetID assetID, QString channelAddress);

private:
    void init();
    bool isRentalChannel(QString channelAddress) const;
    uint64_t getExpiresAt(QString channelAddress) const;
    uint64_t getRentalCreatedDate(QString channelAddress) const;

private:
    ConnextChannels _connextChannels;
    QPointer<ConnextDaemonInterface> _paymentNode;
    QPointer<WalletAssetsModel> _assetsModel;
    QPointer<ChannelRentalHelper> _channelRentalHelper;
};

#endif // CONNEXTCHANNELSLISTMODEL_HPP
