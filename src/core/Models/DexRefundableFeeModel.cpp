#include "DexRefundableFeeModel.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Models/DexService.hpp>
#include <Service/RefundableFeeManager.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

DexRefundableFeeModel::DexRefundableFeeModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

AssetID DexRefundableFeeModel::currentAssetID() const
{
    return _currentAssetID.get_value_or(-1);
}

//==============================================================================

void DexRefundableFeeModel::setCurrentAssetID(AssetID assetID)
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

double DexRefundableFeeModel::refudanbleAmount() const
{
    return _refundableAmount;
}

//==============================================================================

void DexRefundableFeeModel::initialize(ApplicationViewModel* applicationViewModel)
{
    _appViewModel = applicationViewModel;
    _assetsModel = applicationViewModel->assetsModel();

    auto dexService = _appViewModel->dexService();

    if (dexService->isStarted()) {
        onDexServiceReady();
    } else {
        connect(dexService, &DexService::started, this, &DexRefundableFeeModel::onDexServiceReady);
    }
}

//==============================================================================

void DexRefundableFeeModel::onDexServiceReady()
{
    _feeRefunding = _appViewModel->dexService()->feeRefunding()->state();
    connect(_feeRefunding, &swaps::RefundableFeeManagerState::refundableAmountChanged, this,
        [this](std::string currency, swaps::RefundableFeeManagerState::RefundableAmount amount) {
            this->attemtOurCurrency().then([=](std::string ourCurrency) {
                if (ourCurrency == currency) {
                    this->setRefundableAmount(amount.pending + amount.available);
                }
            });
        });
    onAssetIDUpdated();
}

//==============================================================================

void DexRefundableFeeModel::onAssetIDUpdated()
{
    if (!_feeRefunding || !_currentAssetID) {
        return;
    }

    attemtOurCurrency().then([this](std::string currency) {
        QPointer<DexRefundableFeeModel> self(this);
        _feeRefunding->refundableAmountByCurrency(currency).then(
            [self](swaps::RefundableFeeManagerState::RefundableAmount amount) {
                if (self) {
                    self->setRefundableAmount(amount.pending + amount.available);
                }
            });
    });
}

//==============================================================================

void DexRefundableFeeModel::setRefundableAmount(Balance amount)
{
    if (_refundableAmount != amount) {
        _refundableAmount = amount;
        refudanbleAmountChanged();
    }
}

//==============================================================================

Promise<std::string> DexRefundableFeeModel::attemtOurCurrency() const
{
    return QtPromise::attempt(
        [this] { return _assetsModel->assetById(_currentAssetID.get()).ticket().toStdString(); });
}

//==============================================================================
