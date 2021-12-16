#include "ApplicationPaymentNodesViewModel.hpp"
#include <Models/DaemonSlotsManager.hpp>
#include <Models/DexService.hpp>
#include <SwapService.hpp>
#include <Utils/Logging.hpp>

//==============================================================================

ApplicationPaymentNodesViewModel::ApplicationPaymentNodesViewModel(
    DaemonSlotsManager& daemonsManager, DexService& dexService, QObject* parent)
    : QObject(parent)
    , _daemonsManager(daemonsManager)
    , _dexService(dexService)
{
}

//==============================================================================

ApplicationPaymentNodesViewModel::~ApplicationPaymentNodesViewModel() {}

//==============================================================================

bool ApplicationPaymentNodesViewModel::paymentNodeActivity(AssetID assetID)
{
    return _daemonsManager.paymentNodeActivity(assetID);
}

//==============================================================================

Promise<void> ApplicationPaymentNodesViewModel::activateLndsForPair(
    std::vector<AssetID> lndsToActivate)
{
    auto active = _daemonsManager.active();
    for (auto lnd : lndsToActivate) {
        auto it = std::find(active.begin(), active.end(), lnd);
        if (it != std::end(active)) {
            active.erase(it);
            lndsToActivate.erase(std::remove(lndsToActivate.begin(), lndsToActivate.end(), lnd),
                lndsToActivate.end());
        }
    }

    return _daemonsManager.deactivate(active)
        .then([this, lndsToActivate] { _daemonsManager.activate(lndsToActivate); })
        .fail([] { LogCCritical(Lnd) << "Activate lnds failed"; });
}

//==============================================================================

void ApplicationPaymentNodesViewModel::activatePaymentNode(std::vector<AssetID> assetsIDForActivate)
{
    _daemonsManager.activate(assetsIDForActivate);
    _dexService.addCurrencies(assetsIDForActivate).then([this] {
        _dexService.lndsChanged(std::nullopt);
        nodeActivityChanged();
    });
}

//==============================================================================

void ApplicationPaymentNodesViewModel::requestFreeSlots()
{
    freeSlotsFinished(static_cast<int>(_daemonsManager.freeSlots()));
}

//==============================================================================

void ApplicationPaymentNodesViewModel::changeNodeActivity(
    std::vector<int> deactivateLndList, std::vector<int> lndForActivate)
{
    if (deactivateLndList.empty()) {
        auto freeSlots = _daemonsManager.freeSlots();
        auto active = _daemonsManager.active();

        if (freeSlots > 0) {
            activatePaymentNode(std::vector<AssetID>(lndForActivate.begin(), lndForActivate.end()));
        } else {
            noFreeSlots(std::vector<int>(active.begin(), active.end()));
        }
    } else {
        _dexService.deactivateActivePair().then([=] {
            _daemonsManager
                .deactivate(
                    std::vector<AssetID>(deactivateLndList.begin(), deactivateLndList.end()))
                .then([this, lndForActivate] {
                    activatePaymentNode(
                        std::vector<AssetID>(lndForActivate.begin(), lndForActivate.end()));
                })
                .fail([this] { lndActivityChangeFailed("Deactivate lnd error!"); });
        });
    }
}

//==============================================================================

void ApplicationPaymentNodesViewModel::changeSwapAssets(
    AssetID sellAssetID, AssetID buyAssetID, QString pairId)
{
    if (_dexService.currentActivatedPair().pairId != pairId) {
        activateLndsForPair({ sellAssetID, buyAssetID })
            .then([this, sellAssetID, buyAssetID, pairId] {
                _dexService.activatePair(pairId)
                    .then([this, sellAssetID, buyAssetID] {
                        _dexService.lndsChanged({ { sellAssetID, buyAssetID } });
                        nodeActivityChanged();
                    })
                    .fail([this](std::exception& ex) {
                        _dexService.deactivateActivePair();
                        _dexService.lndsChanged(std::nullopt);
                        changeSwapAssetsFailed(ex.what());
                    });
            });
    }
}

//==============================================================================
