#include "LightningSendTransactionViewModel.hpp"
#include <Data/LnPaymentsProxy.hpp>
#include <LndTools/AbstractLndProcessManager.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

//==============================================================================

LightningSendTransactionViewModel::LightningSendTransactionViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

LightningPayRequest LightningSendTransactionViewModel::payRequest() const
{
    return _payRequest;
}

//==============================================================================

void LightningSendTransactionViewModel::initialize(
    AssetID assetID, ApplicationViewModel* applicationViewModel)
{
    _daemonInterface = applicationViewModel->paymentNodesManager()->interfaceById(assetID);

    if (_daemonInterface->type() == Enums::PaymentNodeType::Lnd) {
        _lnPayments
            = applicationViewModel->paymentNodesManager()->lnDaemonManager()->paymentsInterfaceById(
                assetID);
    }
}

//==============================================================================

void LightningSendTransactionViewModel::decodePayRequest(QString payReq)
{

    if (_daemonInterface->type() == Enums::PaymentNodeType::Lnd) {
        QStringList sendResponse = { payReq };
        _encodedPayRequest = payReq;

        qobject_cast<LnDaemonInterface*>(_daemonInterface)
            ->decodePayRequest(payReq)
            .then([this](LightningPayRequest payReq) { setPayRequest(payReq); })
            .fail([this](QString error) {
                _payRequest.destination.clear();
                payRequestError(error);
            });
    }
}

//==============================================================================

void LightningSendTransactionViewModel::makePayment()
{
    if (_payRequest.destination == "") {
        LogCDebug(General) << "Pay request was not parsed";
        return;
    }

    _lnPayments->makePayment(_encodedPayRequest)
        .then([this](QString paymentHash) { transactionSendingFinished(paymentHash); })
        .fail([this](const std::exception& ex) {
            transactionSendingFailed(QString::fromStdString(ex.what()));
        });
}

//==============================================================================

void LightningSendTransactionViewModel::cancelPayment() {}

//==============================================================================

void LightningSendTransactionViewModel::addInvoice(double amoutInSatoshi)
{
    _lnPayments->addInvoice(static_cast<int64_t>(amoutInSatoshi)).then([this](QString payReq) {
        payRequestReady(payReq);
    });
}

//==============================================================================

void LightningSendTransactionViewModel::setPayRequest(LightningPayRequest payReq)
{
    if (_payRequest.paymentHash != payReq.paymentHash) {
        _payRequest = payReq;
        payRequestChanged();
    }
}

//==============================================================================
