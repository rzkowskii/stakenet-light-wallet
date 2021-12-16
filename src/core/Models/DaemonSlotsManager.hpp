#ifndef DAEMONSLOTSMANAGER_HPP
#define DAEMONSLOTSMANAGER_HPP

#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

class DaemonMonitor;
class WalletAssetsModel;
class PaymentNodesManager;

class DaemonSlotsManager : public QObject {
    Q_OBJECT
public:
    explicit DaemonSlotsManager(const WalletAssetsModel& assetsModel,
        const PaymentNodesManager& paymentNodeManager, QObject* parent = nullptr);
    ~DaemonSlotsManager();

    using CanStart = std::function<bool(AssetID)>;

    std::vector<AssetID> active() const;
    bool paymentNodeActivity(AssetID assetID) const;
    size_t freeSlots() const;
    Promise<void> deactivate(std::vector<AssetID> assets);
    void activate(std::vector<AssetID> assetsToActivate);

    void start(CanStart canStart);
    void stop();

signals:
    void started();

private:
    void manageActiveSlots();

private:
    struct Slot {
        AssetID assetID;
        std::unique_ptr<DaemonMonitor> monitor;
    };

    const WalletAssetsModel& _assetsModel;
    const PaymentNodesManager& _paymentNodeManager;
    std::array<Slot, 2> _availableSlots;
    CanStart _canStart;
};

#endif // DAEMONSLOTSMANAGER_HPP
