#include "SendTransactionViewModel.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/ApiClientNetworkingFactory.hpp>
#include <Models/SendAccountTransactionModel.hpp>
#include <Models/SendUTXOTransactionModel.hpp>
#include <Models/WalletDataSource.hpp>
#include <Networking/AbstractWeb3Client.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <QJsonDocument>
#include <QJsonObject>

//==============================================================================

SendTransactionViewModel::SendTransactionViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

SendTransactionViewModel::~SendTransactionViewModel() {}

//==============================================================================

void SendTransactionViewModel::initialize(ApplicationViewModel* applicationViewModel)
{
    _applicationViewModel = applicationViewModel;
}

//==============================================================================

void SendTransactionViewModel::createSendTransaction(QVariantMap payload)
{
    auto type = payload.value("type").toString().toLower();
    if (type == "utxo") {
        auto userSelectedFeeRate = payload.value("userSelectedFeeRate").toDouble();
        auto substractFeeFromAmount = payload.value("substractFeeFromAmount").toBool();
        std::optional<qint64> feeRateInSatsPerKByte = ConvertToSatsPerKByte(userSelectedFeeRate);
        bitcoin::UTXOSendTxParams params(_sendTransactionModel->assetID(),
            payload.value("addressTo").toString(), payload.value("amount").toDouble(),
            feeRateInSatsPerKByte, substractFeeFromAmount);
        _sendTransactionModel->createSendTransaction(params);
    } else if (type == "account") {
        eth::AccountSendTxParams params(_sendTransactionModel->assetID(),
            payload.value("addressTo").toString(), payload.value("amount").toDouble(), "", "", "",
            "", static_cast<Enums::GasType>(payload.value("gasType").toInt()));
        _sendTransactionModel->createSendTransaction(params);
    } else {
        Q_ASSERT_X(false, __FUNCTION__, "Unexpected send tx type");
    }
}

//==============================================================================

void SendTransactionViewModel::cancelSending()
{
    _sendTransactionModel->cancelSending();
}

//==============================================================================

void SendTransactionViewModel::confirmSending()
{
    _sendTransactionModel->confirmSending();
}

//==============================================================================

void SendTransactionViewModel::requestAddressDetails(AssetID assetID)
{
    QPointer<SendTransactionViewModel> self{ this };
    _applicationViewModel->transactionsCache()->cacheById(assetID).then(
        [self, assetID](AbstractTransactionsCache* cache) {
            if (self) {
                auto assetsModel = self->_applicationViewModel->assetsModel();
                const auto& asset = assetsModel->assetById(assetID);

                if (asset.type() == CoinAsset::Type::UTXO) {
                    self->_sendTransactionModel = new SendUTXOTransactionModel{ assetID,
                        assetsModel, self->_applicationViewModel->dataSource(),
                        self->_applicationViewModel->apiClientsFactory(), cache, self };
                } else if (asset.type() == CoinAsset::Type::Account) {
                    auto chainId = *assetsModel->assetById(assetID).params().chainId;
                    auto factory = self->_applicationViewModel->apiClientsFactory();
                    auto web3 = factory->createWeb3Client(chainId);
                    self->_sendTransactionModel = new SendAccountTransactionModel{ assetID,
                        assetsModel, self->_applicationViewModel->accountDataSource(),
                        std::move(web3), cache, self };
                } else {
                    return;
                }
                self->connectSignals();
            }
        });
}

//==============================================================================

bool SendTransactionViewModel::validateAddress(QString input)
{
    return _sendTransactionModel->validateAddress(input);
}

//==============================================================================

void SendTransactionViewModel::resubmit(QString txId)
{
    _sendTransactionModel->resubmitTransaction(txId);
}

//==============================================================================

void SendTransactionViewModel::connectSignals()
{
    connect(_sendTransactionModel, &SendTransactionModel::transactionCreated, this,
        &SendTransactionViewModel::transactionCreated);
    connect(_sendTransactionModel, &SendTransactionModel::transactionCreatingFailed, this,
        &SendTransactionViewModel::transactionCreatingFailed);
    connect(_sendTransactionModel, &SendTransactionModel::transactionSendingFailed, this,
        &SendTransactionViewModel::transactionSendingFailed);
    connect(_sendTransactionModel, &SendTransactionModel::transactionSendingFinished, this,
        &SendTransactionViewModel::transactionSendingFinished);
    connect(_sendTransactionModel, &SendTransactionModel::transactionResubmitted, this,
        &SendTransactionViewModel::transactionResubmitted);
    connect(_sendTransactionModel, &SendTransactionModel::transactionResubmitFailed, this,
        &SendTransactionViewModel::transactionResubmitFailed);
}

//==============================================================================
