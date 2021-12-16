#include "AutopilotModel.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <LndTools/AbstractLndProcessManager.hpp>
#include <LndTools/LndGrpcClient.hpp>
#include <Utils/Logging.hpp>

using namespace autopilotrpc;

//==============================================================================

AutopilotModel::AutopilotModel(AssetID assetID, const WalletAssetsModel &assetsModel, LndGrpcClient* client, QObject* parent)
    : QObject(parent)
    , _client(client)
    , _assetSettings(assetsModel.assetSettings(assetID))
    , _evaluationTimer(new QTimer(this))
    , _assetID(assetID)
{
    auto asset = assetsModel.assetById(assetID);
    _autopilotDefaultSettings = std::make_pair(asset.lndData().defaultSatsPerByte,
        AutopilotDetails(asset.lndData().autopilotDefaultAllocation,
            asset.lndData().autopilotDefaultMaxChannels));

    _evaluationTimer->setInterval(5000);
    _evaluationTimer->setSingleShot(false);
    _evaluationTimer->start(_evaluationTimer->interval());

//    connect(_evaluationTimer, &QTimer::timeout, this, &AutopilotModel::evaluateState);
    connect(_assetSettings, &AssetSettings::autopilotActiveChanged, this, [this](bool active) {
        isActiveChanged();
        _evaluationTimer->start(_evaluationTimer->interval());
        if (active) {
            evaluateDetails();
        } else {
            evaluateState();
        }
    });
    connect(_assetSettings, &AssetSettings::autopilotDetailsChanged, this,
        &AutopilotModel::evaluateDetails);
    //connect(_assetSettings, &AssetSettings::satsPerByteChanged, this, &AutopilotModel::evaluateDetails);
}

//==============================================================================

AutopilotModel::~AutopilotModel() {}

//==============================================================================

void AutopilotModel::setActive(bool active)
{
    _assetSettings->setAutopilotActive(active);
}

//==============================================================================

bool AutopilotModel::isActive() const
{
    return _assetSettings->isAutopilotActive();
}

//==============================================================================

void AutopilotModel::setDetails(double allocation, unsigned maxChannels)
{
    _assetSettings->setAutopilotDetails(AutopilotDetails(allocation, maxChannels));
}

//==============================================================================

double AutopilotModel::allocation() const
{
    auto allocation = _assetSettings->autopilotDetails().allocation;
    return allocation > 0.0 ? allocation : _autopilotDefaultSettings.second.allocation;
}

//==============================================================================

unsigned AutopilotModel::maxChannels() const
{
    auto maxChannels = _assetSettings->autopilotDetails().maxChannels;
    return maxChannels > 0 ? maxChannels : _autopilotDefaultSettings.second.maxChannels;
}

//==============================================================================

void AutopilotModel::evaluateState()
{
    autopilotStatus().then([this](bool status) {
        bool currentState = isActive();
        if (currentState != status) {
            modifyAutopilotStatus(currentState).then([this, currentState] {
                LogCDebug(General) << "Set autopilot status to:" << currentState;
                _evaluationTimer->stop();
            });
        } else {
            _evaluationTimer->stop();
        }
    });
}

//==============================================================================

void AutopilotModel::evaluateDetails()
{
    if (isActive()) {
        auto satsPerByteValue = satsPerByte();
        auto allocationValue = allocation();
        auto maxChannelsValue = maxChannels();
        restartWithNewConstraints(allocationValue, maxChannelsValue, satsPerByteValue)
            .then([allocationValue, maxChannelsValue, satsPerByteValue, this] {
                LogCDebug(General) << "Set autopilot allocation to:" << allocationValue;
                LogCDebug(General) << "Set autopilot maxChannels to:" << maxChannelsValue;
                LogCDebug(General) << "Set autopilot fee rate to:" << satsPerByteValue;
                _evaluationTimer->stop();
            });
    }
}

//==============================================================================

Promise<bool> AutopilotModel::autopilotStatus()
{
    if (_client && _client->isConnected()) {
        return _client
            ->makeAutopilotUnaryRequest<StatusResponse>(
                &Autopilot::Stub::PrepareAsyncStatus, StatusRequest())
            .then([](StatusResponse response) { return response.active(); })
            .fail([](Status status) {
                LogCDebug(Lnd) << "Failed to fetch autopilot status:"
                               << status.error_message().c_str();
                throw;
                return false;
            });
    }

    return Promise<bool>([](const auto&, const auto& reject) { reject(); });
}

//==============================================================================

Promise<void> AutopilotModel::modifyAutopilotStatus(bool active)
{
    if (_client && _client->isConnected()) {
        ModifyStatusRequest req;
        req.set_enable(active);
        return _client
            ->makeAutopilotUnaryRequest<ModifyStatusResponse>(
                &Autopilot::Stub::PrepareAsyncModifyStatus, req)
            .then([](ModifyStatusResponse) -> void {

            })
            .fail([](Status status) {
                LogCDebug(Lnd) << "Failed to modify autopilot status:"
                               << status.error_message().c_str();
                throw;
            });
    }

    return Promise<void>([](const auto&, const auto& reject) { reject(); });
}

//==============================================================================

Promise<void> AutopilotModel::restartWithNewConstraints(
    double allocation, uint32_t maxChannels, uint32_t feeRate)
{
    if (_client && _client->isConnected()) {
        RestartConstraintsRequest req;
        req.set_allocation(allocation);
        req.set_chanlimit(maxChannels);
        req.set_satperbyte(feeRate);

        return _client
            ->makeAutopilotUnaryRequest<RestartConstraintsResponse>(
                &Autopilot::Stub::PrepareAsyncRestartWNewConstraints, req)
            .then([](RestartConstraintsResponse) -> void {})
            .fail([](Status status) {
                LogCDebug(Lnd) << "Failed to restart autopilot, status:"
                               << status.error_message().c_str();
                throw;
            });
    }

    return Promise<void>([](const auto&, const auto& reject) { reject(); });
}

//==============================================================================

unsigned AutopilotModel::satsPerByte() const
{
    auto savedSatsPerByte = _assetSettings->satsPerByte();
    return savedSatsPerByte > 0 ? savedSatsPerByte : _autopilotDefaultSettings.first;
}

//==============================================================================
