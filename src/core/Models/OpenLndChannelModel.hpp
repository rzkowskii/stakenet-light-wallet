#ifndef OPENLNDCHANNELMODEL_HPP
#define OPENLNDCHANNELMODEL_HPP

#include <Models/OpenChannelModel.hpp>
#include <optional>

class LnDaemonInterface;

class OpenLndChannelModel : public OpenChannelModel {
    Q_OBJECT
public:
    explicit OpenLndChannelModel(AssetID assetID, WalletAssetsModel* walletAssetsModel,
        LnDaemonInterface* daemonInterface, QObject* parent = nullptr);

    // OpenChannelModel interface
public:
    void createOpenChannelRequest(
        QString destination, QString localAmount, qint64 networkFee) override;
    void confirmRequest() override;
    void cancelRequest() override;

private:
    QPointer<LnDaemonInterface> _paymentNode;
    struct OpenChannelRequest {
        QString destination;
        QString localAmount;
        qint64 networkFee;
    };
    std::optional<OpenChannelRequest> _currentRequest;
};

#endif // OPENLNDCHANNELMODEL_HPP
