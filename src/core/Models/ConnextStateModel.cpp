#include "ConnextStateModel.hpp"
#include <Models/ConnextDaemonInterface.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/PaymentNodeStateModel.hpp>

//==============================================================================

static const int DEFAULT_IDENTIFIER_SIZE = 56;

//==============================================================================

ConnextStateModel::ConnextStateModel(
    AssetID assetID, ConnextDaemonInterface* daemonInterface, QObject* parent)
    : PaymentNodeStateModel(assetID, parent)
    , _connextManager(daemonInterface)
{
    init();
}

//==============================================================================

ConnextStateModel::~ConnextStateModel() {}

//==============================================================================

ConnextStateModel::ConnextStatus ConnextStateModel::nodeStatus() const
{
    return _connextStatus;
}

//==============================================================================

QString ConnextStateModel::nodeStatusString() const
{
    switch(_connextStatus) {
    case ConnextStatus::ConnextSyncing:
        return "Syncing";
    case ConnextStatus::ConnextNotRunning:
        return "Not running";
    case ConnextStatus::ConnextActive:
        return "Active";
    case ConnextStatus::ConnextNone:
        return "None";
    default:
        "";
    }
}

//==============================================================================

QString ConnextStateModel::nodeStatusColor() const
{
    switch(_connextStatus) {
    case ConnextStatus::ConnextNotRunning:
        return "#A7A8AE";
    case ConnextStatus::ConnextSyncing:
        return "#F19E1E";
    case ConnextStatus::ConnextActive:
        return "#42C451";
    default:
        return "#A7A8AE";
    }
}

//==============================================================================

bool ConnextStateModel::isPendingChannels() const
{
    return false; // TODO: need to impl
}

//==============================================================================

QString ConnextStateModel::identifier() const
{
    return _identifier;
}

//==============================================================================

void ConnextStateModel::setBalance(ConnextDaemonInterface::ConnextBalance balance)
{
    if (_channelsBalance != balance) {
        _channelsBalance = balance;
        nodeBalanceChanged();
        nodeAllBalancesChanged();
    }
}

//==============================================================================

double ConnextStateModel::nodeBalance() const
{
   return static_cast<double>(_channelsBalance.localBalance);
}

//==============================================================================

bool ConnextStateModel::isChannelOpened() const
{
    return _hasChannel;
}

//==============================================================================

Enums::PaymentNodeType ConnextStateModel::nodeType() const
{
    return _paymentNodeType;
}

//==============================================================================

QVariantMap ConnextStateModel::nodeLocalRemoteBalances() const
{
    QVariantMap connextBalances{ { "allLocalBalance", static_cast<double>(_channelsBalance.localBalance) },
        { "allRemoteBalance", static_cast<double>(_channelsBalance.remoteBalance) } };
    return connextBalances;
}

//==============================================================================

void ConnextStateModel::onUpdateConnextStatus()
{
    if (!_connextManager) {
        setConnextStatus(ConnextStatus::ConnextNone);
        return;
    }

    if (!_connextManager->processManager()->running()) {
        setConnextStatus(ConnextStatus::ConnextNotRunning);
    } else {
        setConnextStatus(ConnextStatus::ConnextSyncing);

        if (_identifier.size() == DEFAULT_IDENTIFIER_SIZE) {
            setConnextStatus(ConnextStatus::ConnextActive);
        }
    }
}

//==============================================================================

void ConnextStateModel::init()
{
    connect(_connextManager, &ConnextDaemonInterface::identifierChanged, this,
        &ConnextStateModel::setIdentifier);

    connect(_connextManager, &ConnextDaemonInterface::hasChannelChanged, this,
        &ConnextStateModel::setHasChannel);

    connect(_connextManager->processManager(), &AbstractPaymentNodeProcessManager::runningChanged,
        this, &ConnextStateModel::onUpdateConnextStatus);

    connect(_connextManager, &ConnextDaemonInterface::channelsBalanceChanged, this, &ConnextStateModel::setBalance);
    connect(_connextManager, &ConnextDaemonInterface::identifierChanged, this, &ConnextStateModel::onUpdateConnextStatus);

    _paymentNodeType = Enums::PaymentNodeType::Connext;
    _connextManager->identifier().then([this](QString identifier) { setIdentifier(identifier); });
    _connextManager->isChannelsOpened().then([this](bool value) { setHasChannel(value); });
    _connextManager->channelsBalance().then(
        [this](ConnextDaemonInterface::ConnextBalance newBalance) { setBalance(newBalance); });

    onUpdateConnextStatus();
}

//==============================================================================

void ConnextStateModel::setIdentifier(QString identifier)
{
    if (_identifier != identifier) {
        _identifier = identifier;
        identifierChanged();
    }
}

//==============================================================================

void ConnextStateModel::setConnextStatus(ConnextStateModel::ConnextStatus status)
{
    if (_connextStatus != status) {
        _connextStatus = status;
        nodeStatusChanged();
    }
}

//==============================================================================

void ConnextStateModel::setHasChannel(bool value)
{
    if (_hasChannel != value) {
        _hasChannel = value;
        channelsOpenedChanged();
    }
}

//==============================================================================
