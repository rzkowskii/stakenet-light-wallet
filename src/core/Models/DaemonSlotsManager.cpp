#include "DaemonSlotsManager.hpp"
#include <Data/CoinAsset.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <LndTools/AbstractLndProcessManager.hpp>
#include <Models/LnDaemonInterface.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Tools/DaemonMonitor.hpp>
#include <Utils/Logging.hpp>

//==============================================================================

DaemonSlotsManager::DaemonSlotsManager(const WalletAssetsModel& assetsModel,
    const PaymentNodesManager& paymentNodeManager, QObject* parent)
    : QObject(parent)
    , _assetsModel(assetsModel)
    , _paymentNodeManager(paymentNodeManager)
{
}

//==============================================================================

DaemonSlotsManager::~DaemonSlotsManager()
{
    stop();
}

//==============================================================================

std::vector<AssetID> DaemonSlotsManager::active() const
{
    std::vector<AssetID> assets;
    for (const auto& slot : _availableSlots) {
        if (slot.monitor) {
            assets.push_back(slot.assetID);
        }
    }

    return assets;
}

//==============================================================================

bool DaemonSlotsManager::paymentNodeActivity(AssetID assetID) const
{
    auto activeAssets = active();
    if (_paymentNodeManager.interfaceById(assetID)->type() == Enums::PaymentNodeType::Lnd) {
        return std::find(activeAssets.begin(), activeAssets.end(), assetID) != activeAssets.end();
    } else {
        auto it = std::find_if(activeAssets.begin(), activeAssets.end(), [this](auto coinAssetID) {
            return _paymentNodeManager.interfaceById(coinAssetID)->type() == Enums::PaymentNodeType::Connext;
        });
        return it != activeAssets.end();
    }
}

//==============================================================================

size_t DaemonSlotsManager::freeSlots() const
{
    return std::accumulate(std::begin(_availableSlots), std::end(_availableSlots), 0,
        [](const auto& accum, const auto& slot) { return slot.monitor ? accum : accum + 1; });
}

//==============================================================================

Promise<void> DaemonSlotsManager::deactivate(std::vector<AssetID> assets)
{
    auto deactivateToSlots = std::make_shared<std::vector<Slot>>();
    const auto& all = active();
    auto available = QVector<AssetID>(all.begin(), all.end());
    for (auto&& id : assets) {
        if (!available.contains(id)) {
            return Promise<void>([](const auto&, const auto& reject) { reject(); });
        }

        auto it = std::find_if(std::begin(_availableSlots), std::end(_availableSlots),
            [id](const auto& slot) { return slot.assetID == id; });

        Slot currentSlot;
        std::swap(*it, currentSlot);

        deactivateToSlots->emplace_back(std::move(currentSlot));
    }

    return QtPromise::map(*deactivateToSlots,
        [](const Slot& slot, int) {
            return slot.monitor->stop().then([] { return true; }).fail([] { return false; });
        })
        .then([](const QVector<bool>) {

        })
        .finally([deactivateToSlots]() mutable {
            deactivateToSlots->clear();
            deactivateToSlots.reset();
        });
}

//==============================================================================

void DaemonSlotsManager::activate(std::vector<AssetID> assetsToActivate)
{
    Q_ASSERT_X(assetsToActivate.size() <= _availableSlots.size(), __func__,
        QString("We don't support %1 lnds, current maximum: %2")
            .arg(assetsToActivate.size())
            .arg(_availableSlots.size())
            .toLatin1()
            .data());

    if (freeSlots() < assetsToActivate.size()) {
        LogCDebug(Lnd) << "Don't have enough free slots, can't proceed, have only:" << freeSlots();
        return;
    }

    QVector<AssetID> assets(assetsToActivate.begin(), assetsToActivate.end());
    std::set<AssetID> extCoins;

    for(auto assetID : assets){
        extCoins.emplace(_assetsModel.assetById(assetID).params().params.extCoinType());
    }
    assets = QVector<AssetID>(extCoins.begin(), extCoins.end());;

    for (auto& slot : _availableSlots) {
        if (assets.empty()) {
            break;
        }

        if (!slot.monitor) {
            if (auto daemonInterface = _paymentNodeManager.interfaceById(assets.front())) {
                AssetID assetID ;
                assetID = assets.takeFirst();
                slot.assetID = assetID;
                slot.monitor.reset(new DaemonMonitor(
                    daemonInterface->processManager(), std::bind(_canStart, assetID)));
                slot.monitor->start();
            }
        }
    }
}

//==============================================================================

void DaemonSlotsManager::start(DaemonSlotsManager::CanStart canStart)
{
    _canStart = canStart;
    started();
}

//==============================================================================

void DaemonSlotsManager::stop()
{
    for (auto& slot : _availableSlots) {
        if (slot.monitor) {
            slot.monitor->stop();
        }
        slot.monitor.reset();
    }
}

//==============================================================================
