#include "OpenLndChannelModel.hpp"
#include <Models/LnDaemonInterface.hpp>

//==============================================================================

OpenLndChannelModel::OpenLndChannelModel(AssetID assetID, WalletAssetsModel* walletAssetsModel,
    LnDaemonInterface* daemonInterface, QObject* parent)
    : OpenChannelModel(assetID, walletAssetsModel, parent)
    , _paymentNode(daemonInterface)
{
}

//==============================================================================

void OpenLndChannelModel::createOpenChannelRequest(
    QString destination, QString localAmount, qint64 networkFee)
{
    _currentRequest.emplace(OpenChannelRequest{ destination, localAmount, networkFee });
    emit requestCreated(destination, networkFee, 0);
}

//==============================================================================

void OpenLndChannelModel::confirmRequest()
{
    if (_currentRequest.has_value()) {
        const auto& req = _currentRequest.value();
        QPointer<OpenLndChannelModel> self{this};
        _paymentNode->addChannelRequest(req.destination, req.localAmount, req.networkFee)
            .then([self] {
            if (self) {
                self->channelOpened();
            }
        })
            .fail([self](const std::exception& ex) {
                if (self) {
                    self->channelOpeningFailed(QString::fromStdString(ex.what()));
                }
            })
            .fail([self](const QString& ex) {
                if(self) {
                    self->channelOpeningFailed(ex);
                }
        });
    } else {
        emit channelOpeningFailed("Cannot create channel without request");
    }
}

//==============================================================================

void OpenLndChannelModel::cancelRequest()
{
    _currentRequest.reset();
}

//==============================================================================
