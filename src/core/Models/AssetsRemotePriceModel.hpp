#ifndef ASSETSREMOTEPRICEMODEL_HPP
#define ASSETSREMOTEPRICEMODEL_HPP

#include <Networking/AbstractRemotePriceProvider.hpp>
#include <QObject>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>

class WalletAssetsModel;
class LocalCurrency;

class AssetsRemotePriceModel : public QObject {
    Q_OBJECT
public:
    explicit AssetsRemotePriceModel(WalletAssetsModel& assetsModel,
        qobject_delete_later_unique_ptr<AbstractRemotePriceProvider>&& priceProvider,
        LocalCurrency& localCurrency, QObject* parent = nullptr);

signals:
    void priceUpdated(AssetID assetID, double newPrice);

public slots:
    double exchangeValue(AssetID assetID);
    double exchangeValueByUSD(AssetID assetID);

private:
    void onActiveCurrencyChanged(QString newCurrencyCode);
    void setAssetPrice(AssetID assetID, double price);
    void setAssetUSDPrice(AssetID assetID, double price);
    void onAssetIsActiveChanged(AssetID assetID, bool active);
    void onRefreshPrice();
    void fetchAndSetPrice(AssetID assetID, QString newCurrencyCode);

private:
    WalletAssetsModel& _assetsModel;
    qobject_delete_later_unique_ptr<AbstractRemotePriceProvider> _priceProvider;
    LocalCurrency& _localCurrency;
    QTimer* _refreshTimer{ nullptr };
    std::map<AssetID, std::pair<double, double>> _data;
};

#endif // ASSETSREMOTEPRICEMODEL_HPP
