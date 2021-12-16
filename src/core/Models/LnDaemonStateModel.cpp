#include "LnDaemonStateModel.hpp"
#include <Data/AssetsBalance.hpp>
#include <LndTools/LndProcessManager.hpp>
#include <Models/AutopilotModel.hpp>
#include <Models/LnDaemonInterface.hpp>
#include <Models/PaymentNodeStateModel.hpp>

//==============================================================================

LnDaemonStateModel::LnDaemonStateModel(AssetID assetID, LnDaemonInterface* daemonInterface,
    AutopilotModel* autopilotModel, const AssetsBalance* assetsBalance, QObject* parent)
    : PaymentNodeStateModel(assetID, parent)
    , _lndManager(daemonInterface)
    , _autopilotModel(autopilotModel)
    , _balance(assetsBalance)
{
    init();
}

//==============================================================================

LnDaemonStateModel::~LnDaemonStateModel() {}

//==============================================================================

void LnDaemonStateModel::init()
{
    connect(this, &LnDaemonStateModel::numPeersChanged, this,
        &LnDaemonStateModel::onUpdateAutopilotStatus);
    connect(this, &LnDaemonStateModel::nodeStatusChanged, this,
        &LnDaemonStateModel::onUpdateAutopilotStatus);
    connect(_balance, &AssetsBalance::balanceChanged, this,
        &LnDaemonStateModel::onUpdateAutopilotStatus);

    connect(_lndManager->processManager(), &AbstractLndProcessManager::runningChanged, this,
        &LnDaemonStateModel::onUpdateLndStatus);
    connect(
        this, &LnDaemonStateModel::numPeersChanged, this, &LnDaemonStateModel::onUpdateLndStatus);
    connect(_lndManager, &LnDaemonInterface::balanceChanged, this, &LnDaemonStateModel::setBalance);
    connect(_lndManager, &LnDaemonInterface::hasActiveChannelChanged, this,
        &LnDaemonStateModel::setHasActiveChannels);
    connect(_lndManager, &LnDaemonInterface::pubKeyChanged, this, &LnDaemonStateModel::setPubKey);
    connect(_lndManager, &LnDaemonInterface::numberOfPeersChanged, this,
        &LnDaemonStateModel::setPeersNumber);
    connect(_autopilotModel, &AutopilotModel::isActiveChanged, this,
        &LnDaemonStateModel::autopilotActiveChanged);
    connect(_autopilotModel, &AutopilotModel::isActiveChanged, this,
        &LnDaemonStateModel::onUpdateAutopilotStatus);
    connect(_lndManager, &LnDaemonInterface::chainSyncedChanged, this,
        &LnDaemonStateModel::setChainSynced);
    connect(_lndManager, &LnDaemonInterface::hasPendingChannelChanged, this,
        &LnDaemonStateModel::setHasPendingChannels);

    _paymentNodeType = Enums::PaymentNodeType::Lnd;
    _lndManager->identifier().then([this](QString pubKey) { setPubKey(pubKey); });
    _lndManager->peersNumber().then([this](int peersNum) { setPeersNumber(peersNum); });
    _lndManager->isChannelsOpened().then([this](bool value) { setHasActiveChannels(value); });
    _lndManager->balance().then(
        [this](LnDaemonInterface::LnBalance newBalance) { setBalance(newBalance); });
    _lndManager->syncedToChain().then([this](LnDaemonInterface::ChainSynced synced) {
        setChainSynced(synced.chain, synced.graph);
    });

    autopilotActiveChanged();

    onUpdateLndStatus();
    onUpdateAutopilotStatus();
}

//==============================================================================

bool LnDaemonStateModel::isLndRunning() const
{
    return _lndManager && _lndManager->processManager()->running();
}

//==============================================================================

LnDaemonStateModel::AutopilotStatus LnDaemonStateModel::autopilotStatus() const
{
    return _autopilotStatus;
}

//==============================================================================

bool LnDaemonStateModel::isAutopilotActive() const
{
    return _autopilotModel && _autopilotModel->isActive();
}

//==============================================================================

LnDaemonStateModel::LndStatus LnDaemonStateModel::nodeStatus() const
{
    return _lndStatus;
}

//==============================================================================

QString LnDaemonStateModel::nodeStatusString() const
{
    switch(_lndStatus) {
    case LndStatus::LndNotRunning:
        return "Not running";
    case LndStatus::LndChainSyncing:
        return "Chain syncing";
    case LndStatus::LndGraphSyncing:
        return "Graph syncing";
    case LndStatus::WaitingForPeers:
        return "Waiting for peers";
    case LndStatus::LndActive:
        return "Active";
    case LndStatus::LndNone:
        return "None";
    default:
        return "";
    }
}

//==============================================================================

QString LnDaemonStateModel::nodeStatusColor() const
{
    switch (_lndStatus) {
      case LndStatus::LndActive:
        return "#42C451";
      case LndStatus::LndNotRunning:
         return "#A7A8AE";
      case LndStatus::LndChainSyncing:
         return "#F19E1E";
      case LndStatus::LndGraphSyncing:
          return "#F19E1E";
      case LndStatus::WaitingForPeers:
          return "#F19E1E";
      default:
          return "#A7A8AE";
    }
}

//==============================================================================

QVariantMap LnDaemonStateModel::autopilotDetails() const
{
    QVariantMap result;
    if (_autopilotModel) {
        result["allocation"] = _autopilotModel->allocation();
        result["maxChannels"] = _autopilotModel->maxChannels();
    }
    return result;
}

//==============================================================================

bool LnDaemonStateModel::isPendingChannels() const
{
    return _hasPendingChannels;
}

//==============================================================================

void LnDaemonStateModel::setPubKey(QString pubKey)
{
    if (_pubKey != pubKey) {
        _pubKey = pubKey;
        identifierChanged();
    }
}

//==============================================================================

void LnDaemonStateModel::setPeersNumber(int peersNum)
{
    if (_numPeers != peersNum) {
        _numPeers = peersNum;
        numPeersChanged();
        onUpdateLndStatus();
    }
}

//==============================================================================

void LnDaemonStateModel::setChainSynced(bool syncedToChain, bool syncedToGraph)
{
    _syncedToChain = syncedToChain;
    _syncedToGraph = syncedToGraph;

    onUpdateAutopilotStatus();
    onUpdateLndStatus();
}

//==============================================================================

void LnDaemonStateModel::setHasPendingChannels(bool value)
{
    if (_hasPendingChannels != value) {
        _hasPendingChannels = value;
        channelsPendingChanged();
    }
}

//==============================================================================

void LnDaemonStateModel::setBalance(LnDaemonInterface::LnBalance balance)
{
    if (_allBalance != balance) {
        _allBalance = balance;
        nodeBalanceChanged();
        lndAllBalancesChanged();
        nodeAllBalancesChanged();
    }
}

//==============================================================================

void LnDaemonStateModel::setHasActiveChannels(bool value)
{
    if (_hasActiveChannels != value) {
        _hasActiveChannels = value;
        channelsOpenedChanged();
    }
}

//==============================================================================

void LnDaemonStateModel::setAutopilotStatus(AutopilotStatus status)
{
    if (_autopilotStatus != status) {
        _autopilotStatus = status;
        autopilotStatusChanged();
    }
}

//==============================================================================

void LnDaemonStateModel::setLndStatus(LnDaemonStateModel::LndStatus status)
{
    if (_lndStatus != status) {
        _lndStatus = status;
        nodeStatusChanged();
    }
}

//==============================================================================

void LnDaemonStateModel::onUpdateAutopilotStatus()
{
    if (!_balance || !_lndManager) {
        setAutopilotStatus(AutopilotStatus::AutopilotNone);
        return;
    }

    bool hasEnoughBalance
        = _balance->hasBalance(_assetID) && _balance->balanceById(_assetID).balance > 0;

    bool hasPeers = _numPeers > 0;

    if (!_lndManager->processManager()->running() || !_autopilotModel->isActive()) {
        setAutopilotStatus(AutopilotStatus::AutopilotNone);
    } else if (!_syncedToChain) {
        setAutopilotStatus(AutopilotStatus::AutopilotChainSyncing);
    } else if (!_syncedToGraph) {
        setAutopilotStatus(AutopilotStatus::AutopilotGraphSyncing);
    } else if (!hasPeers) {
        setAutopilotStatus(AutopilotStatus::AutopilotNoPeers);
    } else if (!hasEnoughBalance) {
        setAutopilotStatus(AutopilotStatus::AutopilotNotEnoughCoins);
    } else {
        setAutopilotStatus(AutopilotStatus::AutopilotActive);
    }
}

//==============================================================================

void LnDaemonStateModel::onUpdateLndStatus()
{
    if (!_lndManager) {
        setLndStatus(LndStatus::LndNone);
        return;
    }

    bool hasPeers = _numPeers > 0;

    if (!_lndManager->processManager()->running()) {
        setLndStatus(LndStatus::LndNotRunning);
    } else if (!_syncedToChain) {
        setLndStatus(LndStatus::LndChainSyncing);
    } else if (!_syncedToGraph) {
        setLndStatus(LndStatus::LndGraphSyncing);
    } else if (!hasPeers) {
        setLndStatus(LndStatus::WaitingForPeers);
    } else {
        setLndStatus(LndStatus::LndActive);
    }
}

//==============================================================================

QString LnDaemonStateModel::identifier() const
{
    return _pubKey;
}

//==============================================================================

int LnDaemonStateModel::numPeers() const
{
    return _numPeers;
}

//==============================================================================

double LnDaemonStateModel::nodeBalance() const
{
    return static_cast<double>(_allBalance.total());
}

//==============================================================================

bool LnDaemonStateModel::isChannelOpened() const
{
    return _hasActiveChannels;
}

//==============================================================================

Enums::PaymentNodeType LnDaemonStateModel::nodeType() const
{
    return _paymentNodeType;
}

//==============================================================================

QVariantMap LnDaemonStateModel::lndAllBalances() const
{
    QVariantMap lndBalances{ { "active", static_cast<double>(_allBalance.allActive) },
        { "inactive", static_cast<double>(_allBalance.allInactive) },
        { "pending", static_cast<double>(_allBalance.allPending) },
        { "closing", static_cast<double>(_allBalance.allClosing) } };
    return lndBalances;
}

//==============================================================================

QVariantMap LnDaemonStateModel::nodeLocalRemoteBalances() const
{
    QVariantMap lndBalances{ { "allLocalBalance", static_cast<double>(_allBalance.allLocal) },
        { "allRemoteBalance", static_cast<double>(_allBalance.allRemote) } };
    return lndBalances;
}

//==============================================================================
