#include "PaymentNodeStateManager.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/ConnextStateModel.hpp>
#include <Models/LnDaemonStateModel.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>

PaymentNodeStateManager::PaymentNodeStateManager(const WalletAssetsModel& assetsModel,
    const AssetsBalance& assetsBalance, const PaymentNodesManager& paymentManager, QObject* parent)
    : QObject(parent)
    , _assetsBalance(assetsBalance)
    , _paymentManager(paymentManager)
{
    init(assetsModel);
}

//==============================================================================

PaymentNodeStateManager::~PaymentNodeStateManager() {}

//==============================================================================

PaymentNodeStateModel* PaymentNodeStateManager::stateModelById(AssetID assetID) const
{
    return _stateModels.count(assetID) > 0 ? _stateModels.at(assetID).get() : nullptr;
}

//==============================================================================

Enums::PaymentNodeType PaymentNodeStateManager::stateModelTypeById(AssetID assetID) const
{
    return _paymentManager.interfaceById(assetID)->type();
}

//==============================================================================

void PaymentNodeStateManager::init(const WalletAssetsModel& assetsModel)
{
    for (auto&& assetID : assetsModel.activeAssets()) {
        auto interface = _paymentManager.interfaceById(assetID);
        if (interface->type() == Enums::PaymentNodeType::Lnd) {
            if (auto lndNode = qobject_cast<LnDaemonInterface*>(interface)) {
                _stateModels.emplace(assetID,
                    std::make_unique<LnDaemonStateModel>(assetID, lndNode,
                        _paymentManager.lnDaemonManager()->autopilotById(assetID),
                        &_assetsBalance));
            }
        } else {
            if (auto connextNode = qobject_cast<ConnextDaemonInterface*>(interface)) {
                _stateModels.emplace(
                    assetID, std::make_unique<ConnextStateModel>(assetID, connextNode));
            }
        }
    }
}

//==============================================================================
