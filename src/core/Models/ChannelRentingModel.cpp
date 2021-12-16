#include "ChannelRentingModel.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Models/ChannelRentalHelper.hpp>
#include <Models/DexService.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Models/PaymentNodeInterface.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>
#include <Tools/Common.hpp>

#include <Service/ChannelRentingManager.hpp>
#include <SwapService.hpp>

//==============================================================================

ChannelRentingModel::ChannelRentingModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

AssetID ChannelRentingModel::assetID() const
{
    return _assetID.get_value_or(-1);
}

//==============================================================================

void ChannelRentingModel::setAssetID(AssetID assetId)
{
    if (assetID() != assetId) {
        if (assetId >= 0) {
            _assetID = assetId;
            onAssetIDUpdated();
            assetIDChanged();
        } else {
            _assetID.reset();
        }
    }
}

//==============================================================================

QVariantMap ChannelRentingModel::rentingFee() const
{
    QVariantMap rentingFee;
    rentingFee["fee"] = static_cast<double>(_rentingFee.fee);
    rentingFee["rentingFee"] = static_cast<double>(_rentingFee.rentingFee);
    rentingFee["onChainFee"] = static_cast<double>(_rentingFee.onChainFee);
    return rentingFee;
}

//==============================================================================

double ChannelRentingModel::extendedRentingFee() const
{
    return static_cast<double>(_extendedRentingFee);
}

//==============================================================================

std::vector<QString> ChannelRentingModel::payingAssets() const
{
    std::vector<QString> result;
    if( _assetID && PAYING_CURRENCIES.count(_assetID.get()) && _assetsModel){
        for (auto assetId : PAYING_CURRENCIES.at(_assetID.get())) {
            result.push_back(_assetsModel->assetById(assetId).ticket());
        }
    }
    return result;
}

//==============================================================================

void ChannelRentingModel::initialize(ApplicationViewModel* appViewModel)
{
    _assetsModel = appViewModel->assetsModel();
    _channelRentingManager = appViewModel->dexService()->channelRentingManager();
    _channelRentalHelper = appViewModel->channelRentalHelper();
    _paymentNodesManager = appViewModel->paymentNodesManager();

    onAssetIDUpdated();
}

//==============================================================================

void ChannelRentingModel::rent(AssetID payingAssetID, double capacity, int lifetimeHours)
{
    auto requestedCurrency = _assetsModel->assetById(_assetID.get()).ticket();
    auto payingCurrency = _assetsModel->assetById(payingAssetID).ticket();
    int64_t channelCapacity = static_cast<int64_t>(capacity);
    int64_t lifetimeSeconds = lifetimeHours * 60 * 60;

    Enums::PaymentNodeType payingCurrencyType;
    auto paymentNodeIn = _paymentNodesManager->interfaceById(payingAssetID);
    if (paymentNodeIn) {
        payingCurrencyType = paymentNodeIn->type();
    }

    LogCDebug(Swaps) << "Renting channel, requestedCurrency:" << requestedCurrency
                     << "payingCurrency:" << payingCurrency << "capacity:" << channelCapacity
                     << "lifetimeSeconds:" << lifetimeSeconds;

    QPointer<ChannelRentingModel> self(this);
    _channelRentingManager->rent(payingCurrencyType == Enums::PaymentNodeType::Lnd, requestedCurrency.toStdString(), payingCurrency.toStdString(), channelCapacity,
        lifetimeSeconds)
        .then([self](storage::RentedChannel channel) {
            if (self) {
                LogCDebug(Swaps) << "Successfully rented, channel id:"
                                 << channel.channelid().data();
                self->rentingSuccess();
            }
        })
        .fail([self](const std::exception& ex) {
            if (self) {
                self->rentingFailure(ex.what());
            }
        })
        .fail([self] {
            if (self) {
                self->rentingFailure("Failed to rent channel with unknown reason");
            }
        });
}

//==============================================================================

void ChannelRentingModel::updateRentingFee(
    AssetID payingAssetID, double capacity, int lifetimeHours)
{
    if (_assetsModel) {
        auto requestedCurrency = _assetsModel->assetById(_assetID.get()).ticket().toStdString();
        auto payingCurrency = _assetsModel->assetById(payingAssetID).ticket().toStdString();
        int64_t channelCapacity = static_cast<int64_t>(capacity);
        int64_t lifetimeSeconds = lifetimeHours * 60 * 60;

        QPointer<ChannelRentingModel> self(this);
        _channelRentingManager
            ->feeToRentChannel(requestedCurrency, payingCurrency, channelCapacity, lifetimeSeconds)
            .then([self](orderbook::RentingFee feeInfo) {
                if (!self) {
                    return;
                }
                self->setRentingFee(feeInfo);
            })
            .fail([self](const std::exception& ex) {
                if (!self) {
                    return;
                }
                self->rentingFeeFailure(QString::fromStdString(ex.what()));
            });
    }
}

//==============================================================================

void ChannelRentingModel::extendTime(
    QString fundingTxOutpoint, AssetID payingAssetID, int lifetimeHours)
{
    auto payingCurrency = _assetsModel->assetById(payingAssetID).ticket();
    int64_t lifetimeSeconds = lifetimeHours * 60 * 60;
    auto channelID = _channelRentalHelper->channelIDByTxOutpoint(_assetID.get(), fundingTxOutpoint);

    LogCDebug(Swaps) << "Extending channel, payingAssetID:" << payingAssetID
                     << "lifetimeSeconds:" << lifetimeSeconds;

    QPointer<ChannelRentingModel> self(this);
    _channelRentingManager
        ->extendTime(channelID.toStdString(), payingCurrency.toStdString(), lifetimeSeconds)
        .then([self, lifetimeHours](storage::RentedChannel channel) {
            if (self) {
                LogCDebug(Swaps) << "Succesfully extended, channel id:"
                                 << channel.channelid().data() << "for time " << lifetimeHours
                                 << " hours";
                self->extendingSuccess(static_cast<unsigned>(lifetimeHours));
            }
        })
        .fail([self](const std::exception& ex) {
            if (!self) {
                return;
            }
            LogCCritical(Swaps) << "Extending failure : " << ex.what();
            self->extendingFailure(QString::fromStdString(ex.what()));
        })
        .fail([self] {
            if (!self) {
                return;
            }
            LogCCritical(Swaps) << "Extending failure, unknown reason";
            self->extendingFailure("Failed to extend channel with unknown reason");
        });
}

//==============================================================================

void ChannelRentingModel::updateExtendedRentingFee(
    QString fundingTxOutpoint, AssetID payingAssetID, int lifetimeHours)
{
    if (_assetsModel) {
        auto payingCurrency = _assetsModel->assetById(payingAssetID).ticket().toStdString();
        int64_t lifetimeSeconds = lifetimeHours * 60 * 60;
        auto channelID = _channelRentalHelper->channelIDByTxOutpoint(_assetID.get(), fundingTxOutpoint);

        QPointer<ChannelRentingModel> self(this);
        _channelRentingManager
            ->feeToExtendRentalChannel(channelID.toStdString(), payingCurrency, lifetimeSeconds)
            .then([self](int64_t fee) {
                if (!self) {
                    return;
                }
                self->setExtendRentingFee(fee);
            })
            .fail([self](const std::exception& ex) {
                if (!self) {
                    return;
                }
                self->rentingFeeFailure(QString::fromStdString(ex.what()));
            });
    }
}

//==============================================================================

void ChannelRentingModel::setRentingFee(orderbook::RentingFee newRentingFee)
{
    if (_rentingFee != newRentingFee) {
        _rentingFee = newRentingFee;
        rentingFeeChanged();
    }
}

//==============================================================================

void ChannelRentingModel::setExtendRentingFee(int64_t newExtendRentingFee)
{
    if (_extendedRentingFee != newExtendRentingFee) {
        _extendedRentingFee = newExtendRentingFee;
        extendedRentingFeeChanged();
    }
}

//==============================================================================

void ChannelRentingModel::onAssetIDUpdated()
{
    if (!_paymentNodesManager || !_assetID) {
        return;
    }
    initNodeType();
}

//==============================================================================

void ChannelRentingModel::initNodeType()
{
    auto _paymentNodeIn = _paymentNodesManager->interfaceById(_assetID.get());
    if (_paymentNodeIn) {
        _paymentNodeType = _paymentNodeIn->type();
    }
}

//==============================================================================
