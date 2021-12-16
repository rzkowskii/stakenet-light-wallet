#ifndef OPENCONNEXTCHANNELMODEL_HPP
#define OPENCONNEXTCHANNELMODEL_HPP

#include <Models/OpenChannelModel.hpp>
#include <Utils/Utils.hpp>

#include <optional>

namespace orderbook {
class OrderbookRefundableFees;
}

class QTimer;
class SendTransactionModel;
class ConnextDaemonInterface;
class AssetsBalance;

/*!
 * \brief The OpenConnextChannelModel class implements OpenChannelModel interface.
 * Contains logic for opening connext channel. We require user to pay for connext channel
 * deployment. This model allows user to pay fee for connext channel deployment by making ordinary
 * send transaction to hub as reciever. After transaction was send we pool backend by some fixed
 * interval and wait for it to ack our payment. After we have recieved positive ack we request
 * ConnextDaemonInterface to open channel.
 */
class OpenConnextChannelModel : public OpenChannelModel {
    Q_OBJECT
public:
    explicit OpenConnextChannelModel(AssetID assetID, WalletAssetsModel* walletAssetsModel,
        AssetsBalance* assetsBalance, SendTransactionModel* sendTxModel,
        ConnextDaemonInterface* daemonInterface, orderbook::OrderbookRefundableFees* fees,
        QObject* parent = nullptr);

    // OpenChannelModel interface
public:
    void createOpenChannelRequest(
        QString destination, QString localAmount, qint64 networkFee) override;
    void confirmRequest() override;
    void cancelRequest() override;

private slots:
    void onTransactionCreated(qint64 deploymentFee, qint64 networkFee, QString recipientAddress);

private:
    void openChannel(QString destination, int64_t amount);
    void checkIfCanOpenChannel();
    using DeploymentFeeResponse = std::pair<Balance, std::string>;
    Promise<DeploymentFeeResponse> fetchDeploymentFee();

private:
    struct OpenChannelRequest {
        QString destination;
        Balance amount;
    };

    QPointer<orderbook::OrderbookRefundableFees> _orderbookFees;
    QPointer<SendTransactionModel> _sendTxModel;
    QPointer<ConnextDaemonInterface> _daemonInterface;
    QPointer<QTimer> _openChannelTimer;
    std::optional<OpenChannelRequest> _currentRequest;
    QPointer<AssetsBalance> _assetsBalance;
};

#endif // OPENCONNEXTCHANNELMODEL_HPP
