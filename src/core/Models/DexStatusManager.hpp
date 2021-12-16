#ifndef DexStatusManager_HPP
#define DexStatusManager_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <Models/PaymentNodeStateManager.hpp>
#include <Models/LnDaemonStateModel.hpp>
#include <Models/ConnextStateModel.hpp>

class PaymentNodeStateManager;
class WalletAssetsModel;
class PaymentNodeStateModel;

class DexStatusManager : public QObject {
    Q_OBJECT
public:
    explicit DexStatusManager(QPointer<PaymentNodeStateManager> paymentNodeStateManager,
        QPointer<WalletAssetsModel> assetModel, QObject* parent = nullptr);
    ~DexStatusManager();

    Enums::DexTradingStatus dexStatusById(AssetID id) const;

signals:
    void dexStatusChanged(AssetID assetID);

private slots:
    void onUpdateDexStatus(AssetID assetID);

private:
    void init();
    void initTradeAssets();
    void initStateModels();
    Enums::DexTradingStatus getStatus(AssetID assetID);
    Enums::DexTradingStatus getLndTradeStatus(LnDaemonStateModel::AutopilotStatus autopilotStatus,  LnDaemonStateModel::LndStatus lndStatus, bool isChannelOpened);
    Enums::DexTradingStatus getConnextTradeStatus(ConnextStateModel::ConnextStatus connextStatus, bool isChannelOpened);


private:
    QPointer<WalletAssetsModel> _assetModel;
    QPointer<PaymentNodeStateManager> _paymentNodeStateManager;
    std::map<AssetID, Enums::DexTradingStatus> _dexStatuses;
    std::map<AssetID, std::vector<AssetID>> _availableTradingAssetMap;
    std::map<AssetID, QPointer<PaymentNodeStateModel>> _paymentNodeStateModels;
};

#endif // DexStatusManager_HPP
