#include "AssetsRemotePriceModel.hpp"
#include <Data/LocalCurrency.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Networking/AbstractRemotePriceProvider.hpp>

//==============================================================================

AssetsRemotePriceModel::AssetsRemotePriceModel(WalletAssetsModel& assetsModel,
    qobject_delete_later_unique_ptr<AbstractRemotePriceProvider>&& priceProvider,
    LocalCurrency& localCurrency, QObject* parent)
    : QObject(parent)
    , _assetsModel(assetsModel)
    , _priceProvider(std::move(priceProvider))
    , _localCurrency(localCurrency)
    , _refreshTimer(new QTimer(this))
{

    for (auto&& assetID : _assetsModel.activeAssets()) {
        _data.emplace(assetID, std::make_pair<double, double>(0.0, 0.0));
    }

    for (auto&& asset : _assetsModel.assets()) {
        auto assetID = asset.coinID();
        auto settings = _assetsModel.assetSettings(assetID);
        connect(settings, &AssetSettings::activeChanged, this,
            [this, assetID](bool active) { onAssetIsActiveChanged(assetID, active); });
    }

    connect(&_localCurrency, &LocalCurrency::activeCurrencyChanged, this,
        &AssetsRemotePriceModel::onActiveCurrencyChanged);
    connect(_refreshTimer, &QTimer::timeout, this, &AssetsRemotePriceModel::onRefreshPrice);

    _refreshTimer->setInterval(1000 * 60 * 10);
    _refreshTimer->setSingleShot(false);
    _refreshTimer->start(_refreshTimer->interval());

    onActiveCurrencyChanged(_localCurrency.activeCurrency().code);
}

//==============================================================================

double AssetsRemotePriceModel::exchangeValue(AssetID assetID)
{
    return _data.count(assetID) > 0 ? _data.at(assetID).first : 0.0;
}

//==============================================================================

double AssetsRemotePriceModel::exchangeValueByUSD(AssetID assetID)
{
    return _data.count(assetID) > 0 ? _data.at(assetID).second : 0.0;
}

//==============================================================================

void AssetsRemotePriceModel::onActiveCurrencyChanged(QString newCurrencyCode)
{
    for (auto& it : _data) {
        fetchAndSetPrice(it.first, newCurrencyCode);
    }
}
//==============================================================================

void AssetsRemotePriceModel::setAssetUSDPrice(AssetID assetID, double newPrice)
{
    if (_data.count(assetID) == 0) {
        return;
    }

    if (!qFuzzyCompare(_data.at(assetID).second, newPrice)) {
        _data[assetID].second = newPrice;
        priceUpdated(assetID, newPrice);
    }
}

//==============================================================================

void AssetsRemotePriceModel::setAssetPrice(AssetID assetID, double newPrice)
{
    if (_data.count(assetID) == 0) {
        return;
    }

    if (!qFuzzyCompare(_data.at(assetID).first, newPrice)) {
        _data[assetID].first = newPrice;
        priceUpdated(assetID, newPrice);
    }
}

//==============================================================================

void AssetsRemotePriceModel::onAssetIsActiveChanged(AssetID assetID, bool active)
{
    if (active) {
        _data[assetID].first = 0.0;
        _data[assetID].second = 0.0;
        fetchAndSetPrice(assetID, _localCurrency.activeCurrency().code);
    } else {
        if (_data.count(assetID) > 0) {
            _data.erase(assetID);
        }
    }
}

//==============================================================================

void AssetsRemotePriceModel::onRefreshPrice()
{
    for (auto&& it : _data) {
        fetchAndSetPrice(it.first, _localCurrency.activeCurrency().code);
    }
}

//==============================================================================

void AssetsRemotePriceModel::fetchAndSetPrice(AssetID assetID, QString newCurrencyCode)
{
    auto currencySymbol = _assetsModel.assetById(assetID).ticket().toLower();

    if (assetID == 9999 || assetID == 9999001
        || assetID == 88) { // when ropsten and rinkeby enabled for testing
        setAssetPrice(assetID, 0.0);
        setAssetUSDPrice(assetID, 0.0);
        return;
    }
    _priceProvider->fetchPrice(currencySymbol, newCurrencyCode.toLower())
        .then([this, assetID](double newValue) { setAssetPrice(assetID, newValue); })
        .fail([this, assetID]() {
            if (assetID == 60003) {
                this->setAssetPrice(assetID, 1.0);
            } else {
                this->setAssetPrice(assetID, 0.0);
            }
        });

    _priceProvider->fetchPrice(currencySymbol, "usd")
        .then([this, assetID](double newValue) { setAssetUSDPrice(assetID, newValue); })
        .fail([this, assetID]() {
            if (assetID == 60003) {
                this->setAssetUSDPrice(assetID, 1.0);
            } else {
                this->setAssetUSDPrice(assetID, 0.0);
            }
        });
}

//==============================================================================
