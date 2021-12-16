#ifndef LOCALCURRENCYVIEWMODEL_HPP
#define LOCALCURRENCYVIEWMODEL_HPP

#include <QObject>
#include <Tools/Common.hpp>

class AssetsRemotePriceModel;
class LocalCurrency;

class LocalCurrencyViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentCurrencyCode READ currentCurrencyCode NOTIFY localCurrencyChanged)
    Q_PROPERTY(QString currentCurrencySymbol READ currentCurrencySymbol NOTIFY localCurrencyChanged)
public:
    explicit LocalCurrencyViewModel(AssetsRemotePriceModel& currencyRates,
        LocalCurrency& localCurrency, QObject* parent = nullptr);
    ~LocalCurrencyViewModel();

    QString currentCurrencyCode() const;
    QString currentCurrencySymbol() const;

    Balance convertSats(AssetID assetID, Balance coinBalance);

signals:
    void localCurrencyChanged();
    void currencyRateChanged(AssetID assetID);

public slots:
    QString convert(AssetID assetID, QString coinBalance);

    void changeLocalCurrency(QString code);
    QString convertToCoins(AssetID assetID, QString localBalance);
    QString convertUSDToCoins(AssetID assetID, QString usdBalance);

private:
    AssetsRemotePriceModel& _currencyRates;
    LocalCurrency& _localCurrency;
};

#endif // LOCALCURRENCYVIEWMODEL_HPP
