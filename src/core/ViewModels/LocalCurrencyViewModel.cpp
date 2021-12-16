#include "LocalCurrencyViewModel.hpp"
#include <Data/LocalCurrency.hpp>
#include <Models/AssetsRemotePriceModel.hpp>

//==============================================================================

LocalCurrencyViewModel::LocalCurrencyViewModel(
    AssetsRemotePriceModel& currencyRates, LocalCurrency& localCurrency, QObject* parent)
    : QObject(parent)
    , _currencyRates(currencyRates)
    , _localCurrency(localCurrency)
{
    connect(&_currencyRates, &AssetsRemotePriceModel::priceUpdated, this,
        [this](AssetID assetID, double) { currencyRateChanged(assetID); });
}

//==============================================================================

LocalCurrencyViewModel::~LocalCurrencyViewModel() {}

//==============================================================================

QString LocalCurrencyViewModel::currentCurrencyCode() const
{
    return _localCurrency.activeCurrency().code;
}

//==============================================================================

QString LocalCurrencyViewModel::currentCurrencySymbol() const
{
    return _localCurrency.activeCurrency().symbol;
}

//==============================================================================

Balance LocalCurrencyViewModel::convertSats(AssetID assetID, Balance coinBalance)
{
    return _currencyRates.exchangeValue(assetID) * coinBalance;
}

//==============================================================================

QString LocalCurrencyViewModel::convert(AssetID assetID, QString coinBalance)
{
    auto localBalance = _currencyRates.exchangeValue(assetID) * coinBalance.toDouble();
    auto minValue = 0.01;

    if (localBalance < minValue && localBalance > 0) {
        return QString("< %1").arg(minValue);
    }

    return QString::number(localBalance, 'f', 2);
}

//==============================================================================

void LocalCurrencyViewModel::changeLocalCurrency(QString code)
{
    if (!code.isEmpty() && _localCurrency.activeCurrency().code != code) {
        _localCurrency.setActiveCurrency(code);
        localCurrencyChanged();
    }
}

//==============================================================================

QString LocalCurrencyViewModel::convertToCoins(AssetID assetID, QString localBalance)
{
    return QString::number(localBalance.toDouble() / _currencyRates.exchangeValue(assetID), 'f', 8);
}

//==============================================================================

QString LocalCurrencyViewModel::convertUSDToCoins(AssetID assetID, QString usdBalance)
{
    return QString::number(
        usdBalance.toDouble() / _currencyRates.exchangeValueByUSD(assetID), 'f', 8);
}

//==============================================================================
