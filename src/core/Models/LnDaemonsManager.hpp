#ifndef LNDAEMONSMANAGER_HPP
#define LNDAEMONSMANAGER_HPP

#include <QObject>
#include <array>
#include <functional>
#include <memory>
#include <queue>
#include <unordered_map>

#include <Models/LnDaemonInterface.hpp>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

class AssetsTransactionsCache;
class DaemonMonitor;
class AutopilotModel;
class WalletAssetsModel;
class LndBackupManager;
class LnPaymentsProxy;

class LnDaemonsManager : public QObject {
    Q_OBJECT
public:
    explicit LnDaemonsManager(Utils::WorkerThread& workerThread, AssetsTransactionsCache& txCache,
        const WalletAssetsModel& assetsModel, QObject* parent = nullptr);
    ~LnDaemonsManager();

    LnDaemonInterface* interfaceById(AssetID assetID) const;
    AutopilotModel* autopilotById(AssetID assetID) const;
    LndBackupManager* backupManagerById(AssetID assetID) const;
    LnPaymentsProxy* paymentsInterfaceById(AssetID assetID) const;

private:
    void init(AssetsTransactionsCache& txCache);

private:
    Utils::WorkerThread& _workerThread;
    const WalletAssetsModel& _assetsModel;
    std::unordered_map<AssetID, qobject_delete_later_unique_ptr<LnDaemonInterface>>
        _daemonsInterfaces;
    std::unordered_map<AssetID, qobject_delete_later_unique_ptr<AutopilotModel>> _autopilotModels;
    std::unordered_map<AssetID, qobject_delete_later_unique_ptr<LndBackupManager>> _backupManagers;
    std::unordered_map<AssetID, qobject_delete_later_unique_ptr<LnPaymentsProxy>>
        _paymentSyncManagers;
};

#endif // LNDAEMONSMANAGER_HPP
