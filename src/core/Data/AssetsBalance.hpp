#ifndef ASSETSBALANCE_HPP
#define ASSETSBALANCE_HPP

#include <Data/AbstractAssetBalanceProvider.hpp>
#include <Data/TransactionEntry.hpp>
#include <Models/WalletDataSource.hpp>
#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>
#include <unordered_map>

class WalletAssetsModel;
class LocalCurrencyViewModel;
class AbstractChainManager;
struct AbstractAssetBalanceProviderFactory;

class AssetsBalance : public QObject {
    Q_OBJECT
public:
    using BalanceMap = std::unordered_map<AssetID, AssetBalance>;
    using BalanceLocalMap = std::unordered_map<AssetID, double>;

    explicit AssetsBalance(QPointer<WalletAssetsModel> walletAssetsModel,
        QPointer<LocalCurrencyViewModel> localCurrencyModel,
        QPointer<AbstractChainManager> chainManager,
        std::unique_ptr<AbstractAssetBalanceProviderFactory> factory, QObject* parent = nullptr);
    virtual ~AssetsBalance();

    const BalanceMap& balance() const;
    const BalanceLocalMap& localBalance() const;

    Balance balanceSum() const;
    double balanceLocalSum() const;
    bool hasBalance(AssetID assetID) const;
    AssetBalance balanceById(AssetID assetID) const;
    double balanceLocalById(AssetID assetID) const;
    void update();

signals:
    void localBalanceChanged(AssetID assetID);
    void balanceChanged(AssetID assetID);
    void confirmedBalanceChanged(AssetID assetID);

private slots:
    void onLocalCurrencyChanged();
    void onCurrencyRateChanged(AssetID assetID);
    void updateBalance(AssetID assetID, AssetBalance balance);

private:
    void init();
    void createProvider(AssetID assetID);

private:
    using BalanceProviderRef = std::unique_ptr<AbstractAssetBalanceProvider>;
    QPointer<WalletAssetsModel> _walletAssetsModel;
    QPointer<LocalCurrencyViewModel> _localCurrencyModel;
    QPointer<AbstractChainManager> _chainManager;
    std::unordered_map<AssetID, BalanceProviderRef> _providers;
    std::unique_ptr<AbstractAssetBalanceProviderFactory> _factory;
    BalanceMap _assetsBalance;
    BalanceLocalMap _assetsLocalBalance;
};

#endif // ASSETSBALANCE_HPP
