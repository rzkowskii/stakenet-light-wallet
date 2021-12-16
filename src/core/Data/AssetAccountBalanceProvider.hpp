#ifndef ASSETACCOUNTBALANCEPROVIDER_HPP
#define ASSETACCOUNTBALANCEPROVIDER_HPP

#include <Data/AbstractAssetBalanceProvider.hpp>
#include <Data/CoinAsset.hpp>
#include <QObject>

class AssetsAccountsCache;
class ConnextDaemonInterface;

class AssetAccountBalanceProvider : public AbstractAssetBalanceProvider {
    Q_OBJECT
public:
    explicit AssetAccountBalanceProvider(CoinAsset coinAsset,
        QPointer<AssetsAccountsCache> accountsCache, QPointer<ConnextDaemonInterface> connextInterface, QObject* parent = nullptr);

    // AbstractAssetBalanceProvider interface
public:
    void update() override;

private slots:
    void onBalanceChanged(Balance newBalance);

private:
    void init(QPointer<AssetsAccountsCache> accountsCache);

private:
    CoinAsset _asset;
    QPointer<ConnextDaemonInterface> _connextInterface;
};

#endif // ASSETACCOUNTBALANCEPROVIDER_HPP
