#include "PaymentNodeViewModel.hpp"
#include <LndTools/LndProcessManager.hpp>
#include <Models/AutopilotModel.hpp>
#include <Models/ConnextChannelsListModel.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/ConnextStateModel.hpp>
#include <Models/DexService.hpp>
#include <Models/LnDaemonStateModel.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/LndChannelsListModel.hpp>
#include <Models/PaymentNodeStateManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <SwapService.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

PaymentNodeViewModel::PaymentNodeViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

PaymentNodeViewModel::~PaymentNodeViewModel() {}

//==============================================================================

void PaymentNodeViewModel::initialize(ApplicationViewModel* applicationViewModel)
{
    _applicationViewModel = applicationViewModel;
    _hostList = new QStringListModel();
    onAssetIDUpdated();
}

////==============================================================================

//void PaymentNodeViewModel::openChannel(
//    QString identityKey, QString localAmount, unsigned feeSatsPerByte)
//{
//    Balance amt = localAmount.toDouble() * COIN;

//    openNodeChannel(identityKey, amt, feeSatsPerByte)
//        .then([this](QString) { this->channelOpened(); })
//        .fail([this](const std::exception& ex) {
//            this->channelFailedToOpen(QString::fromStdString(ex.what()));
//        })
//        .fail([this](const QString& ex) { this->channelFailedToOpen(ex); });
//}

//==============================================================================

bool PaymentNodeViewModel::isLightningAddress(QString address)
{
    return address.length() > 32 && address.startsWith("ln");
}

//==============================================================================

void PaymentNodeViewModel::closeAllChannels(unsigned feeSatsPerByte)
{
    _paymentNodeIn->closeAllChannels(feeSatsPerByte)
        .then([this] {
            cancelOrders();
            allChannelsClosed();
        })
        .fail([this]() { allChannelsFailedToClose("Can`t close lightning channels!"); });
}

//==============================================================================

void PaymentNodeViewModel::closeChannel(
    QString channelId, bool isChannelInactive, unsigned feeSatsPerByte)
{
    if (_paymentNodeType == Enums::PaymentNodeType::Lnd) {
        qobject_cast<LnDaemonInterface*>(_paymentNodeIn)
            ->closeChannel(channelId, isChannelInactive, feeSatsPerByte)
            .then([this] {
                if (!_paymentNodeStateModel->isChannelOpened()) {
                    cancelOrders();
                }

                LogCDebug(Lnd) << "Lightning closeChannel";
                channelClosed();
            })
            .fail([this](std::string errStatus) {
                LogCCritical(Lnd) << "Lightning channelFailedToClose" << errStatus.c_str();
                channelFailedToClose("Can`t close lightning channel!");
            });
    } else {
    }
}

//==============================================================================

void PaymentNodeViewModel::activateAutopilot(bool active)
{
    _applicationViewModel->paymentNodesManager()
        ->lnDaemonManager()
        ->autopilotById(_currentAssetID.get())
        ->setActive(active);
}

//==============================================================================

void PaymentNodeViewModel::changeAutopilotDetails(double allocation, unsigned maxChannels)
{
    _applicationViewModel->paymentNodesManager()
        ->lnDaemonManager()
        ->autopilotById(_currentAssetID.get())
        ->setDetails(allocation, maxChannels);
}

//==============================================================================

void PaymentNodeViewModel::changeApPreOpenPopupVisibility()
{
    _apPreOpenPopupVisibility = !_apPreOpenPopupVisibility;
}

//==============================================================================

bool PaymentNodeViewModel::apPreOpenPopupVisibility()
{
    return _apPreOpenPopupVisibility;
}

//==============================================================================

void PaymentNodeViewModel::invoicePaymentRequestByPreImage(QString rHash)
{
    if (_paymentNodeType == Enums::PaymentNodeType::Lnd) {
        qobject_cast<LnDaemonInterface*>(_paymentNodeIn)
            ->invoicePaymentRequestByPreImage(rHash.toStdString())
            .then([this](std::string payReq) { lnInvoiceReady(QString::fromStdString(payReq)); });
    } else {
        throw("Connext does not support getting invoice payment request by preImage");
    }
}

//==============================================================================

void PaymentNodeViewModel::depositChannel(QString amount, QString channelAddress)
{
    if (_paymentNodeType == Enums::PaymentNodeType::Connext) {
        Balance amt = amount.toDouble() * COIN;
        qobject_cast<ConnextDaemonInterface*>(_paymentNodeIn)
            ->depositChannel(amt, channelAddress, _currentAssetID.get())
            .then([this](QString channelAddress) { this->depositChannelFinished(channelAddress); })
            .fail([this](const std::exception& ex) {
                this->depositChannelFailed(QString::fromStdString(ex.what()));
            })
            .fail([this](const QString& ex) { this->depositChannelFailed(ex); });
    } else {
        throw("Lnd does not support channel deposit");
    }
}

//==============================================================================

void PaymentNodeViewModel::withdraw(
    QString recipientAddress, QString amount, QString channelAddress)
{
    if (_paymentNodeType == Enums::PaymentNodeType::Connext) {
        Balance amt = amount.toDouble() * COIN;
        qobject_cast<ConnextDaemonInterface*>(_paymentNodeIn)
            ->withdraw(recipientAddress, amt, channelAddress, _currentAssetID.get())
            .then([this]() { this->withdrawFinished(); })
            .fail([this](const std::exception& ex) {
                this->withdrawFailed(QString::fromStdString(ex.what()));
            })
            .fail([this](const QString& ex) { this->withdrawFailed(ex); });
    } else {
        throw("Lnd does not support withdraw channel");
    }
}

//==============================================================================

void PaymentNodeViewModel::reconcileChannel(QString channelAddress)
{
    if (_paymentNodeType == Enums::PaymentNodeType::Connext) {
        qobject_cast<ConnextDaemonInterface*>(_paymentNodeIn)
            ->reconcileChannel(channelAddress, _currentAssetID.get())
            .then([this]() { this->reconcileFinished(); })
            .fail([this](const std::exception& ex) {
                this->reconcileFailed(QString::fromStdString(ex.what()));
            })
            .fail([this](const QString& ex) { this->reconcileFailed(ex); });
    } else {
        throw("Lnd does not support reconcile channel");
    }
}

//==============================================================================

void PaymentNodeViewModel::restoreChannel()
{
    if (_paymentNodeType == Enums::PaymentNodeType::Connext) {
        qobject_cast<ConnextDaemonInterface*>(_paymentNodeIn)
            ->restoreChannel(_currentAssetID.get())
            .then([this](QString channelAddress) { this->restoreChannelFinished(); })
            .fail([this](const std::exception& ex) {
                this->restoreChannelFailed(QString::fromStdString(ex.what()));
            })
            .fail([this](const QString& ex) { this->restoreChannelFailed(ex); });
    } else {
        throw("Lnd does not support restore channel");
    }
}

//==============================================================================

void PaymentNodeViewModel::onAssetIDUpdated()
{
    if (!_applicationViewModel || !_currentAssetID) {
        return;
    }
    initPaymentNode();
    initChannelsModel();
}

//==============================================================================

void PaymentNodeViewModel::cancelOrders()
{
    auto dexService = _applicationViewModel->dexService();
    auto currentActivatedPairId = dexService->currentActivatedPair().pairId;

    if (!currentActivatedPairId.isEmpty()) {
        dexService->swapService().cancelAllOrders(currentActivatedPairId.toStdString());
        LogCDebug(Lnd) << "PaymentNodeViewModel removed own orders";
    }
}

//==============================================================================

void PaymentNodeViewModel::initPaymentNode()
{
    _paymentNodeIn
        = _applicationViewModel->paymentNodesManager()->interfaceById(_currentAssetID.get());
    _paymentNodeStateModel
        = _applicationViewModel->paymentNodeStateManager()->stateModelById(_currentAssetID.get());
    if (_paymentNodeIn) {
        _paymentNodeType = _paymentNodeIn->type();
        _hostList->setStringList(_paymentNodeIn->processManager()->getNodeConf());
        hostModelChanged();
    }
    emit stateModelChanged();
}

//==============================================================================

void PaymentNodeViewModel::initChannelsModel()
{
    Q_ASSERT(_paymentNodeIn);
    _paymentChannelsListModel.reset();
    if (_paymentNodeType == Enums::PaymentNodeType::Lnd) {
        _paymentChannelsListModel = std::make_unique<LndChannelsListModel>(*_currentAssetID,
            _applicationViewModel->transactionsCache(), _applicationViewModel->chainDataSource(),
            _applicationViewModel->chainManager(), _applicationViewModel->channelRentalHelper(),
            _paymentNodeIn);
    } else if (_paymentNodeType == Enums::PaymentNodeType::Connext) {
        _paymentChannelsListModel
            = std::make_unique<ConnextChannelsListModel>(*_currentAssetID, _paymentNodeIn,
                _applicationViewModel->assetsModel(), _applicationViewModel->channelRentalHelper());
    }

    emit channelsModelChanged();
}

////==============================================================================

//Promise<QString> PaymentNodeViewModel::openNodeChannel(
//    QString identityKey, Balance amount, unsigned feeSatsPerByte)
//{
//    if (_paymentNodeType == Enums::PaymentNodeType::Lnd) {
//        return qobject_cast<LnDaemonInterface*>(_paymentNodeIn)
//            ->addChannelRequest(identityKey, QString::number(amount), feeSatsPerByte);
//    } else {
//        return qobject_cast<ConnextDaemonInterface*>(_paymentNodeIn)
//            ->openChannel(identityKey, amount, _currentAssetID.get());
//    }
//}

//==============================================================================

AssetID PaymentNodeViewModel::currentAssetID() const
{
    return _currentAssetID.get_value_or(-1);
}

//==============================================================================

void PaymentNodeViewModel::setCurrentAssetID(int assetID)
{
    if (currentAssetID() != assetID) {
        if (assetID >= 0) {
            _currentAssetID = assetID;
            onAssetIDUpdated();
            currentAssetIDChanged();
        } else {
            _currentAssetID.reset();
        }
    }
}

//==============================================================================

PaymentNodeStateModel* PaymentNodeViewModel::stateModel() const
{
    return _paymentNodeStateModel;
}

//==============================================================================

QStringListModel* PaymentNodeViewModel::hostModel() const
{
    return _hostList;
}

//==============================================================================

Enums::PaymentNodeType PaymentNodeViewModel::type() const
{
    return _paymentNodeType;
}

//==============================================================================

PaymentChannelsListModel* PaymentNodeViewModel::channelsModel() const
{
    return _paymentChannelsListModel.get();
}

//==============================================================================
