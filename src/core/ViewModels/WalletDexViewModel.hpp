#ifndef WALLETDEXVIEWMODEL_HPP
#define WALLETDEXVIEWMODEL_HPP

#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>
#include <memory>

class DexService;
class WalletAssetsModel;
class ApplicationViewModel;
class WalletDexStateModel;

class WalletDexViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool hasSwapPair READ hasSwapPair NOTIFY swapAssetsChanged)
    Q_PROPERTY(AssetID quoteAssetID READ quoteAssetID NOTIFY swapAssetsChanged)
    Q_PROPERTY(AssetID baseAssetID READ baseAssetID NOTIFY swapAssetsChanged)
    Q_PROPERTY(bool online READ isOnline NOTIFY isOnlineChanged)
    Q_PROPERTY(bool inMaintenance READ inMaintenance NOTIFY isInMaintenanceChanged)
    Q_PROPERTY(bool canPlaceOrder READ canPlaceOrder NOTIFY canPlaceOrderChanged)
    Q_PROPERTY(QVariantMap tradingPairInfo READ tradingPairInfo NOTIFY swapAssetsChanged)
    Q_PROPERTY(WalletDexStateModel* stateModel READ stateModel NOTIFY stateModelChanged)

public:
    using SwapAssets = std::pair<AssetID, AssetID>;

    explicit WalletDexViewModel(QObject* parent = nullptr);
    ~WalletDexViewModel();

    QVariantMap tradingPairInfo() const;

    AssetID quoteAssetID() const;
    AssetID baseAssetID() const;
    bool isOnline() const;
    bool hasSwapPair() const;
    bool canPlaceOrder() const;
    bool inMaintenance() const;

    WalletDexStateModel* stateModel() const;

signals:
    void swapAssetsChanged();
    void isOnlineChanged();
    void swapSuccess(double amount, QString receiveSymbol);
    void swapFailed(QString errorText);
    void placeOrderFailed(QString errorText);
    void placeOrderSucceeded();
    void canPlaceOrderChanged();
    void isInMaintenanceChanged();
    void stateModelChanged();

public slots:
    void initialize(ApplicationViewModel* appViewModel);
    void createOrder(QString amount, QString total, double price, bool isBuy);
    bool placeOrderBoxVisibility();
    void changePlaceOrderBoxVisibility();
    void cancelOrder(QString orderID);
    void cancelAllOrders();

private slots:
    void onDexServiceReady();

private:
    void reportNoRouteError(AssetID assetID);
    void setCanPlaceOrder(bool value);

private:
    QPointer<WalletAssetsModel> _walletAssetsModel;
    QPointer<DexService> _dexService;

    bool _placeOrderBoxVisibility{ true };
    bool _canPlaceOrder{ true };
};

#endif // WALLETDEXVIEWMODEL_HPP
