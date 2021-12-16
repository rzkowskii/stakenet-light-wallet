#include "DexStatusManager.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Models/ConnextStateModel.hpp>
#include <Models/LnDaemonStateModel.hpp>
#include <Models/PaymentNodeStateManager.hpp>
#include <Models/PaymentNodeStateModel.hpp>

#include <algorithm>

static const AssetID BITCOIN_ID = 0;

//==============================================================================

DexStatusManager::DexStatusManager(QPointer<PaymentNodeStateManager> paymentNodeStateManager,
    QPointer<WalletAssetsModel> assetModel, QObject* parent)
    : QObject(parent)
    , _assetModel(assetModel)
    , _paymentNodeStateManager(paymentNodeStateManager)
{
    init();
}

//==============================================================================

DexStatusManager::~DexStatusManager() {}

//==============================================================================

Enums::DexTradingStatus DexStatusManager::dexStatusById(AssetID id) const
{
    if (_dexStatuses.count(id) > 0) {
        return _dexStatuses.at(id);
    }

    return Enums::DexTradingStatus::Offline;
}

//==============================================================================

void DexStatusManager::onUpdateDexStatus(AssetID assetID)
{
    if (_availableTradingAssetMap.count(assetID) == 0) {
        return;
    }

    auto lndStatus = getStatus(assetID);
    std::vector<std::pair<AssetID, Enums::DexTradingStatus>> lndStatuses;

    for (auto asset : _availableTradingAssetMap.at(assetID)) {
        lndStatuses.push_back({ asset, getStatus(asset) });
    }

    auto predicate = [](const auto& pair, auto status) { return pair.second == status; };

    auto syncingIt = std::find_if(lndStatuses.begin(), lndStatuses.end(),
        std::bind(predicate, std::placeholders::_1, Enums::DexTradingStatus::Syncing));
    auto onlineIt = std::find_if(lndStatuses.begin(), lndStatuses.end(),
        std::bind(predicate, std::placeholders::_1, Enums::DexTradingStatus::Online));

    bool isAnySyncing = syncingIt != lndStatuses.end();
    bool isAnyOnline = onlineIt != lndStatuses.end();

    Enums::DexTradingStatus newTradingStatus{ Enums::DexTradingStatus::Offline };

    if (lndStatus == Enums::DexTradingStatus::Online) {
        newTradingStatus = isAnyOnline
            ? Enums::DexTradingStatus::Online
            : (isAnySyncing ? Enums::DexTradingStatus::Syncing : Enums::DexTradingStatus::Offline);
    } else if (lndStatus == Enums::DexTradingStatus::Syncing) {
        newTradingStatus = isAnyOnline || isAnySyncing ? Enums::DexTradingStatus::Syncing
                                                       : Enums::DexTradingStatus::Offline;
    }

    if (dexStatusById(assetID) != newTradingStatus) {
        _dexStatuses[assetID] = newTradingStatus;
        dexStatusChanged(assetID);
    }
}

//==============================================================================

void DexStatusManager::init()
{
    initTradeAssets();
    initStateModels();

    connect(this, &DexStatusManager::dexStatusChanged, this, [this](AssetID assetID) {
        for (auto&& it : _availableTradingAssetMap) {
            if (std::find(std::begin(it.second), std::end(it.second), assetID)
                != std::end(it.second)) {
                // if asset was changed that is part of our supported pairs, be sure to update
                // status
                onUpdateDexStatus(it.first);
            }
        }
    });
}

//==============================================================================

void DexStatusManager::initTradeAssets()
{
    for (auto asset : _assetModel->activeAssets()) {
        if (asset == BITCOIN_ID) {
            auto activeAssets = _assetModel->activeAssets();
            activeAssets.erase(std::remove(activeAssets.begin(), activeAssets.end(), BITCOIN_ID),
                activeAssets.end());
            activeAssets.erase(std::remove(activeAssets.begin(), activeAssets.end(), 60001),
                activeAssets.end());
            activeAssets.erase(std::remove(activeAssets.begin(), activeAssets.end(), 60002),
                activeAssets.end());
            _availableTradingAssetMap.emplace(BITCOIN_ID, activeAssets);
        } else {
            std::vector<AssetID> list;
            list.push_back(BITCOIN_ID);
            _availableTradingAssetMap.emplace(asset, list);
        }

        _dexStatuses.emplace(asset, Enums::DexTradingStatus::Offline);
    }
}

//==============================================================================

void DexStatusManager::initStateModels()
{
    for (auto asset : _assetModel->activeAssets()) {
        if (auto stateModel = _paymentNodeStateManager->stateModelById(asset)) {
            auto stateModelType = _paymentNodeStateManager->stateModelTypeById(asset);

            _paymentNodeStateModels.emplace(asset, stateModel);
            connect(stateModel, &PaymentNodeStateModel::channelsOpenedChanged, this,
                std::bind(&DexStatusManager::onUpdateDexStatus, this, asset));

            if (stateModelType == Enums::PaymentNodeType::Lnd) {
                auto lndStateModel = qobject_cast<LnDaemonStateModel*>(stateModel);
                connect(lndStateModel, &LnDaemonStateModel::autopilotStatusChanged, this,
                    std::bind(&DexStatusManager::onUpdateDexStatus, this, asset));

                connect(lndStateModel, &LnDaemonStateModel::nodeStatusChanged, this,
                    std::bind(&DexStatusManager::onUpdateDexStatus, this, asset));
            } else {
                auto connextStateModel = qobject_cast<ConnextStateModel*>(stateModel);
                connect(connextStateModel, &ConnextStateModel::nodeStatusChanged, this,
                    std::bind(&DexStatusManager::onUpdateDexStatus, this, asset));
            }
        }
    }
}

//==============================================================================

Enums::DexTradingStatus DexStatusManager::getStatus(AssetID assetID)
{
    if (_paymentNodeStateModels.count(assetID) > 0) {
        auto paymentNodeStateModel = _paymentNodeStateModels.at(assetID);
        if (_paymentNodeStateManager->stateModelTypeById(assetID) == Enums::PaymentNodeType::Lnd) {
            auto tradeStateModel = qobject_cast<LnDaemonStateModel*>(paymentNodeStateModel);
            return getLndTradeStatus(tradeStateModel->autopilotStatus(), tradeStateModel->nodeStatus(),
                tradeStateModel->isChannelOpened());

        } else {
            auto tradeStateModel = qobject_cast<ConnextStateModel*>(paymentNodeStateModel);
            return getConnextTradeStatus(tradeStateModel->nodeStatus(),
                tradeStateModel->isChannelOpened());
        }
    }

    return Enums::DexTradingStatus::Offline;
}

//==============================================================================

Enums::DexTradingStatus DexStatusManager::getLndTradeStatus(
    LnDaemonStateModel::AutopilotStatus autopilotStatus, LnDaemonStateModel::LndStatus lndStatus,
    bool isChannelOpened)
{
    switch (autopilotStatus) {
    case LnDaemonStateModel::AutopilotStatus::AutopilotActive:
        return isChannelOpened ? Enums::DexTradingStatus::Online : Enums::DexTradingStatus::Offline;
    case LnDaemonStateModel::AutopilotStatus::AutopilotChainSyncing:
        return Enums::DexTradingStatus::Syncing;
    case LnDaemonStateModel::AutopilotStatus::AutopilotGraphSyncing:
        return Enums::DexTradingStatus::Syncing;
    default:
        break;
    }

    switch (lndStatus) {
    case LnDaemonStateModel::LndStatus::LndActive:
        return isChannelOpened ? Enums::DexTradingStatus::Online : Enums::DexTradingStatus::Offline;
    case LnDaemonStateModel::LndStatus::LndChainSyncing:
        return Enums::DexTradingStatus::Syncing;
    case LnDaemonStateModel::LndStatus::LndGraphSyncing:
        return Enums::DexTradingStatus::Syncing;
    default:
        return Enums::DexTradingStatus::Offline;
    }
}

//==============================================================================

Enums::DexTradingStatus DexStatusManager::getConnextTradeStatus(
    ConnextStateModel::ConnextStatus connextStatus, bool isChannelOpened)
{
    switch (connextStatus) {
    case ConnextStateModel::ConnextStatus::ConnextActive:
        return isChannelOpened ? Enums::DexTradingStatus::Online : Enums::DexTradingStatus::Offline;
    case ConnextStateModel::ConnextStatus::ConnextSyncing:
        return Enums::DexTradingStatus::Syncing;
    default:
        return Enums::DexTradingStatus::Offline;
    }
}

//==============================================================================
