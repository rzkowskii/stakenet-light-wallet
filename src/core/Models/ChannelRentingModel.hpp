#ifndef CHANNELRENTINGMODEL_HPP
#define CHANNELRENTINGMODEL_HPP

#include <QObject>
#include <Service/ChannelRentingManager.hpp>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <boost/optional.hpp>
#include <QPointer>

class ApplicationViewModel;
class LnDaemonsManager;
class WalletAssetsModel;
class ChannelRentalHelper;
class PaymentNodesManager;

namespace orderbook {
class ChannelRentingManager;
struct RentingFee;
}

class ChannelRentingModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(AssetID assetID READ assetID WRITE setAssetID NOTIFY assetIDChanged)
    Q_PROPERTY(QVariantMap rentingFee READ rentingFee NOTIFY rentingFeeChanged)
    Q_PROPERTY(double extendedRentingFee READ extendedRentingFee NOTIFY extendedRentingFeeChanged)
    Q_PROPERTY(std::vector<QString> payingAssets READ payingAssets NOTIFY assetIDChanged)

public:
    explicit ChannelRentingModel(QObject* parent = nullptr);

    AssetID assetID() const;
    void setAssetID(AssetID assetID);

    QVariantMap rentingFee() const;
    double extendedRentingFee() const;
    std::vector<QString> payingAssets() const;

public slots:
    void initialize(ApplicationViewModel* appViewModel);
    void rent(AssetID payingAssetID, double capacity, int lifetimeHours);
    void updateRentingFee(AssetID payingAssetID, double capacity, int lifetimeHours);
    void extendTime(QString fundingTxOutpoint, AssetID payingAssetID, int lifetimeHours);
    void updateExtendedRentingFee(
        QString fundingTxOutpoint, AssetID payingAssetID, int lifetimeHours);

signals:
    void assetIDChanged();
    void rentingSuccess();
    void rentingFailure(QString errorString);
    void rentingFeeChanged();
    void rentingFeeFailure(QString errorString);
    void extendingSuccess(unsigned hours);
    void extendingFailure(QString errorString);
    void extendedRentingFeeChanged();

private:
    void setRentingFee(orderbook::RentingFee newRentingFee);
    void setExtendRentingFee(int64_t newExtendRentingFee);
    void onAssetIDUpdated();
    void initNodeType();

private:
    WalletAssetsModel* _assetsModel{ nullptr };
    orderbook::ChannelRentingManager* _channelRentingManager{ nullptr };
    orderbook::RentingFee _rentingFee;
    boost::optional<AssetID> _assetID;
    QPointer<ChannelRentalHelper> _channelRentalHelper;
    int64_t _extendedRentingFee{ 0 };
    Enums::PaymentNodeType _paymentNodeType;
    QPointer<PaymentNodesManager> _paymentNodesManager;
};

#endif // CHANNELRENTINGMODEL_HPP
