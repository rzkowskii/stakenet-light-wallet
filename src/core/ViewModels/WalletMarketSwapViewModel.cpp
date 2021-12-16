#include "WalletMarketSwapViewModel.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Models/WalletMarketSwapModel.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

WalletMarketSwapViewModel::WalletMarketSwapViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

WalletMarketSwapViewModel::~WalletMarketSwapViewModel() {}

//==============================================================================

bool WalletMarketSwapViewModel::marketSwapInProcess() const
{
    if (_marketSwapModel) {
        return _marketSwapModel->currentSwapRequest().has_value();
    }
    return false;
}

//==============================================================================

bool WalletMarketSwapViewModel::sendChannelState() const
{
    if (_marketSwapModel) {
        return _marketSwapModel->currentSwapRequest()->sendChannelOpened.has_value();
    }
    return false;
}

//==============================================================================

bool WalletMarketSwapViewModel::recvChannelState() const
{
    if (_marketSwapModel) {
        return _marketSwapModel->currentSwapRequest()->recvChannelOpened.has_value();
    }
    return false;
}

//==============================================================================

void WalletMarketSwapViewModel::initialize(ApplicationViewModel* appViewModel)
{
    _marketSwapModel = appViewModel->marketSwapModel();
    _assetsModel = appViewModel->assetsModel();

    connect(_marketSwapModel, &WalletMarketSwapModel::swapExecuted, this,
        [this](MarketSwapRequest request) {
            this->swapSuccess(
                request.receiveAmount, _assetsModel->assetById(request.receiveAssetID).ticket());
        });
    connect(_marketSwapModel, &WalletMarketSwapModel::swapFailed, this,
        &WalletMarketSwapViewModel::onSwapFailed);
    connect(_marketSwapModel, &WalletMarketSwapModel::currentSwapRequestChanged, this,
        &WalletMarketSwapViewModel::onCurrentSwapRequestChanged);
}

//==============================================================================

void WalletMarketSwapViewModel::executeSwap(MarketSwapRequest request, double feeSatsPerByte)
{
    auto fee = static_cast<Balance>(feeSatsPerByte);
    _marketSwapModel->executeSwap(request, fee);
}

//==============================================================================

void WalletMarketSwapViewModel::calculatePreSwapAmounts(AssetID sendAssetID, AssetID receiveAssetID,
    double sendAmount, double receiveAmount, double sendAssetFeeRate, double receiveAssetFeeRate,
    bool needToRentChannel)
{
    auto amountToSend = static_cast<Balance>(sendAmount);
    auto amountToReceive = static_cast<Balance>(receiveAmount);
    auto sendFeeRateFee = static_cast<Balance>(sendAssetFeeRate);
    auto receiveFeeRateFee = static_cast<Balance>(receiveAssetFeeRate);

    _marketSwapModel->channelReserve(sendAssetID, amountToSend, sendFeeRateFee)
        .then([this, sendAssetID, receiveAssetID, amountToReceive, receiveFeeRateFee,
                  needToRentChannel](Balance sendReserve) {
            _marketSwapModel->channelReserve(receiveAssetID, amountToReceive, receiveFeeRateFee)
                .then([this, sendAssetID, receiveAssetID, amountToReceive, sendReserve,
                          needToRentChannel](Balance receiveReserve) {
                    if (needToRentChannel) {
                        _marketSwapModel->rentalFee(receiveAssetID, sendAssetID, amountToReceive)
                            .then([this, sendReserve, receiveReserve](Balance fee) {
                                this->calculatePreSwapAmountsFinished(
                                    static_cast<Balance>(fee), sendReserve, receiveReserve);
                            })
                            .fail([this]() {
                                this->rentalFeeFailed("Rental fee for channel failed!");
                            });
                    } else {
                        this->calculatePreSwapAmountsFinished(0, sendReserve, receiveReserve);
                    }
                });
        });
}

//==============================================================================

void WalletMarketSwapViewModel::evaluateChannels(AssetID sendAssetID, AssetID receiveAssetID,
    double sendAmount, double receiveAmount, bool isBuy, double placeOrderFee, double sendResv,
    double receiveResv)
{
    auto amountToSend = static_cast<Balance>(sendAmount);
    auto amountToReceive = static_cast<Balance>(receiveAmount);
    auto orderFee = static_cast<Balance>(placeOrderFee);
    auto sendReserve = static_cast<Balance>(sendResv);
    auto receiveReserve = static_cast<Balance>(receiveResv);

    _marketSwapModel
        ->evaluateChannels(sendAssetID, receiveAssetID, amountToSend, amountToReceive, isBuy,
            orderFee, sendReserve, receiveReserve)
        .then([this](MarketSwapRequest request) { this->evaluateChannelsRequested(request); })
        .fail([this](std::exception(&ex)) {
            this->evaluateChannelsFailed(QString::fromStdString(ex.what()));
        })
        .fail([this]() { this->evaluateChannelsFailed("Evaluate channels failed!"); });
}

//==============================================================================

void WalletMarketSwapViewModel::onCurrentSwapRequestChanged()
{
    emit marketSwapInProcessChanged();
}

//==============================================================================

void WalletMarketSwapViewModel::onSwapFailed(MarketSwapRequest request)
{
    emit swapFailed(*request.failureMsg);
}

//==============================================================================
