#include "OpenConnextChannelModel.hpp"
#include <Data/AssetsBalance.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <EthCore/Types.hpp>
#include <Models/ConnextDaemonInterface.hpp>
#include <Models/SendAccountTransactionModel.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/OrderbookRefundableFees.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>

#include <QTimer>
#include <boost/multiprecision/cpp_dec_float.hpp>

//==============================================================================

OpenConnextChannelModel::OpenConnextChannelModel(AssetID assetID,
    WalletAssetsModel* walletAssetsModel, AssetsBalance* assetsBalance,
    SendTransactionModel* sendTxModel, ConnextDaemonInterface* daemonInterface,
    orderbook::OrderbookRefundableFees* fees, QObject* parent)
    : OpenChannelModel(assetID, walletAssetsModel, parent)
    , _orderbookFees(fees)
    , _sendTxModel(sendTxModel)
    , _daemonInterface(daemonInterface)
    , _assetsBalance(assetsBalance)
{
    connect(_sendTxModel, &SendTransactionModel::transactionCreated, this,
        &OpenConnextChannelModel::onTransactionCreated);
    connect(_sendTxModel, &SendTransactionModel::transactionCreatingFailed, this,
        [this](QString errorMsg) {
            _currentRequest.reset();
            emit this->requestCreatingFailed(
                QString("Could not create open channel tx: %1").arg(errorMsg));
        });
    // whenever we have succesfully sent payment tx we need to start pooling backend for
    // positive ack
    connect(_sendTxModel, &SendTransactionModel::transactionSendingFinished, this, [this] {
        LogCCInfo(General) << "Succesfully sent fee transaction, starting pooling";
        // here we need to wait for tx to become confirmed and only then
        _openChannelTimer = new QTimer{ this };
        _openChannelTimer->setInterval(25 * 1000); // 25 seconds
        _openChannelTimer->setSingleShot(false);
        connect(_openChannelTimer, &QTimer::timeout, this,
            &OpenConnextChannelModel::checkIfCanOpenChannel);
        _openChannelTimer->start();
    });
    connect(_sendTxModel, &SendTransactionModel::transactionSendingFailed, this,
        [this](QString errorMsg) {
            _currentRequest.reset();
            emit this->requestCreatingFailed(
                QString("Could not send open channel tx: %1").arg(errorMsg));
        });
}

//==============================================================================

void OpenConnextChannelModel::createOpenChannelRequest(
    QString destination, QString localAmount, qint64 networkFee)
{
    LogCCInfo(General) << "Requested to create channel to" << destination
                       << "amount:" << localAmount;
    Balance channelAmount = localAmount.toDouble() * COIN;
    fetchDeploymentFee()
        .then([this, destination, channelAmount](DeploymentFeeResponse response) {
            // if backend allows us to open channel, we shouldn't send any transactions, just
            // proceed with opening channel.

            auto [feeAmount, address] = response;

            if (feeAmount == 0) {
                this->openChannel(destination, channelAmount);
                return;
            }

            // cache request data to use it later when we will open channel.
            _currentRequest = OpenChannelRequest{ destination, feeAmount };

            auto assetID = this->assetID();
            auto token = _walletAssetsModel->assetById(assetID).token();
            auto denomination = eth::ether;
            // we will always use ETH as parent chain since all fees are paid in ETH
            if (token) {
                assetID = token->chainID();
            }

            // backend returns all values as 10^8, we need to covert it into target denomination
            // since we are dealing with ETH tokens.

            auto convertedFee = eth::ConvertDenominations(eth::u256{ feeAmount }, 8, 18);

            boost::multiprecision::cpp_dec_float_50 wei(convertedFee);
            wei /= static_cast<boost::multiprecision::cpp_dec_float_50>(denomination);
            auto feeAsDouble = wei.convert_to<double>();

            // when creating send TX it will be multiplied by denomination
            eth::AccountSendTxParams params(assetID, QString::fromStdString(address), feeAsDouble,
                "", "", "", "", Enums::GasType::Average);
            _sendTxModel->createSendTransaction(params);
        })
        .tapFail([this](QString errorMsg) {
            emit this->requestCreatingFailed(
                QString("Could not fetch deployment fee: %1").arg(errorMsg));
        });
}

//==============================================================================

void OpenConnextChannelModel::confirmRequest()
{
    _sendTxModel->confirmSending();
}

//==============================================================================

void OpenConnextChannelModel::cancelRequest()
{
    _currentRequest.reset();
    _sendTxModel->cancelSending();
}

//==============================================================================

void OpenConnextChannelModel::onTransactionCreated(
    qint64 deploymentFee, qint64 networkFee, QString recipientAddress)
{
    auto assetID = this->assetID();
    auto asset =_walletAssetsModel->assetById(assetID);
    auto token = asset.token();
    // we will always use ETH as parent chain since all fees are paid in ETH
    if (token) {
        assetID = token->chainID();
    }
    auto totalFee = networkFee + deploymentFee;
    if (!_assetsBalance->hasBalance(assetID) || _assetsBalance->balanceById(assetID).confirmedBalance < totalFee) {
            LogCCInfo(General) << "requestCreatingFailed" << assetID << QString("Not enough funds to open channel, total fee = %1 %2").arg(FormatAmount(totalFee)).arg(asset.ticket().toUpper());
            requestCreatingFailed(QString("Not enough funds to open channel, total fee = %1 %2").arg(FormatAmount(totalFee)).arg(asset.ticket().toUpper()));
    } else {
        emit this->requestCreated(recipientAddress, networkFee, deploymentFee);
    }
}

//==============================================================================

void OpenConnextChannelModel::openChannel(QString destination, int64_t localAmount)
{
    LogCCInfo(General) << "Opening connext channel to" << destination << "amount:" << localAmount;
    _daemonInterface->openChannel(destination, localAmount, assetID())
        .then([this]() {
            if (_openChannelTimer) {
                _openChannelTimer->stop();
                _openChannelTimer->deleteLater();
            }
            _currentRequest.reset();
            emit this->channelOpened();
        })
        .fail([this](QString errorMsg) {
            this->channelOpeningFailed(QString("Could not open channel: %1").arg(errorMsg));
        })
        .fail([this] {
            emit this->channelOpeningFailed("Failed to open channel due to unknown error");
        });
}

//==============================================================================

void OpenConnextChannelModel::checkIfCanOpenChannel()
{
    LogCCInfo(General) << "Polling if can open channel:" << _currentRequest.has_value();
    if (!_currentRequest) {
        return;
    }
    fetchDeploymentFee()
        .then([this](DeploymentFeeResponse response) {
            if (!_currentRequest) {
                return;
            }
            const auto& req = *_currentRequest;
            auto feeAmount = response.first;
            LogCCInfo(General) << "Polled fee amount:" << feeAmount;
            if (feeAmount == 0) {
                this->openChannel(req.destination, req.amount);
            }
        })
        .tapFail([](QString errorMsg) {
            LogCWarning(General) << "could not poll deployment fee status:" << errorMsg;
        });
}

//==============================================================================

auto OpenConnextChannelModel::fetchDeploymentFee() -> Promise<DeploymentFeeResponse>
{
    return Promise<DeploymentFeeResponse>([this](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_orderbookFees, [=] {
            _orderbookFees->getConnextContractDeploymentFeeResponse()
                .then([resolve](orderbook::OrderbookRefundableFees::
                              ConnextChannelContractDeploymentFeeResponse response) {
                    auto feeAmount = orderbook::ConvertBigInt(response.amount());
                    LogCCInfo(General) << "Requested fee for opening channel:" << feeAmount
                                       << response.amount().value().data();
                    resolve(std::make_pair(feeAmount, response.hubaddress()));
                })
                .fail([reject](std::string reason) { reject(QString::fromStdString(reason)); })
                .fail([reject](std::exception& ex) { reject(QString::fromStdString(ex.what())); })
                .fail([reject] { reject(QString("Unknown refundable fee error")); });
        });
    });
}

//==============================================================================
