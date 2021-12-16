#ifndef APPLICATIONLNDVIEWMODEL_HPP
#define APPLICATIONLNDVIEWMODEL_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

class DaemonSlotsManager;
class DexService;

class ApplicationPaymentNodesViewModel : public QObject {
    Q_OBJECT
public:
    explicit ApplicationPaymentNodesViewModel(
        DaemonSlotsManager& daemonsManager, DexService& dexService, QObject* parent = nullptr);
    ~ApplicationPaymentNodesViewModel();

signals:
    void freeSlotsFinished(int freeSlots);
    void nodeActivityChanged();
    void lndActivityChangeFailed(QString errorMessage);
    void noFreeSlots(std::vector<int> lnds);
    void changeSwapAssetsFailed(QString error);

public slots:
    bool paymentNodeActivity(AssetID assetID);
    void requestFreeSlots();
    void changeNodeActivity(std::vector<int> deactivateLndList, std::vector<int> lndForActivate);
    void changeSwapAssets(AssetID sellAssetID, AssetID buyAssetID, QString pairId);

private:
    Promise<void> activateLndsForPair(std::vector<AssetID> lndsToActivate);
    void activatePaymentNode(std::vector<AssetID> assetsIDForActivate);

private:
    DaemonSlotsManager& _daemonsManager;
    DexService& _dexService;
};

#endif // APPLICATIONLNDVIEWMODEL_HPP
