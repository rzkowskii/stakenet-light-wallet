#include "WalletDexViewModel.hpp"

#include <Data/WalletAssetsModel.hpp>
#include <Models/DexService.hpp>
#include <Models/WalletDexStateModel.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <SwapService.hpp>
#include <Tools/AppConfig.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <utilstrencodings.h>

//==============================================================================

WalletDexViewModel::WalletDexViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

WalletDexViewModel::~WalletDexViewModel() {}

//==============================================================================

void WalletDexViewModel::reportNoRouteError(AssetID assetID)
{
    placeOrderFailed(QString("No route to hub found. Please check your %1 setup")
                         .arg(_walletAssetsModel->assetById(assetID).ticket().toUpper()));
}

//==============================================================================

void WalletDexViewModel::setCanPlaceOrder(bool value)
{
    if (_canPlaceOrder != value) {
        _canPlaceOrder = value;
        canPlaceOrderChanged();
    }
}

//==============================================================================

void WalletDexViewModel::createOrder(QString amount, QString total, double price, bool isBuy)
{
    // pair XSN/LTC, base = XSN, quote = LTC
    // amount is always in base currency
    // total is always in quote currency

    Balance baseQuantity = qRound64(amount.toDouble() * COIN);
    Balance quoteQuantity = qRound64(total.toDouble() * COIN);

    this->setCanPlaceOrder(false);

    _dexService->stateModel()
        .createOrder(baseQuantity, quoteQuantity, price, isBuy)
        .then([this] { this->placeOrderSucceeded(); })
        .tapFail([this](const std::exception& ex) { this->placeOrderFailed(ex.what()); })
        .finally([this] { this->setCanPlaceOrder(true); });
}

//==============================================================================

bool WalletDexViewModel::placeOrderBoxVisibility()
{
    return _placeOrderBoxVisibility;
}

//==============================================================================

void WalletDexViewModel::changePlaceOrderBoxVisibility()
{
    _placeOrderBoxVisibility = !_placeOrderBoxVisibility;
}

//==============================================================================

void WalletDexViewModel::cancelOrder(QString orderID)
{
    _dexService->swapService()
        .cancelOrder(_dexService->stateModel().getPairId().toStdString(), orderID.toStdString())
        .fail([](const std::exception& ex) {
            LogCCritical(Orderbook) << "Failed to cancel order:" << ex.what();
        });
}

//==============================================================================

void WalletDexViewModel::cancelAllOrders()
{
    if (_dexService->stateModel().hasSwapPair()) {
        _dexService->swapService().cancelAllOrders(
            _dexService->stateModel().getPairId().toStdString());
    }
}

//==============================================================================

void WalletDexViewModel::onDexServiceReady()
{
    connect(&_dexService->swapService(), &swaps::SwapService::swapExecuted, this,
        [this](swaps::SwapSuccess info) {
            this->swapSuccess(static_cast<double>(info.amountReceived),
                QString::fromStdString(info.currencyReceived));
        });
    connect(&_dexService->swapService(), &swaps::SwapService::swapFailed, this,
        [this](swaps::SwapFailure info) {
            if (info.role == swaps::SwapRole::Maker) {
                this->swapFailed(ErrorMessageFromSwapFailure(info));
            }
        });

    connect(_dexService->orderbook(), &orderbook::OrderbookClient::ownOrderPlaced, this,
        std::bind(&WalletDexViewModel::setCanPlaceOrder, this, true));

    connect(
        _dexService, &DexService::isConnectedChanged, this, &WalletDexViewModel::isOnlineChanged);
    connect(_dexService, &DexService::isInMaintenanceChanged, this,
        &WalletDexViewModel::isInMaintenanceChanged);
    connect(&_dexService->stateModel(), &WalletDexStateModel::noHubRouteError, this,
        &WalletDexViewModel::reportNoRouteError);
    connect(&_dexService->stateModel(), &WalletDexStateModel::swapAssetsChanged, this,
        &WalletDexViewModel::swapAssetsChanged);

    isOnlineChanged();
    isInMaintenanceChanged();
}

//==============================================================================

QVariantMap WalletDexViewModel::tradingPairInfo() const
{
    QVariantMap result;
    if (!hasSwapPair()) {
        return result;
    }

    const auto& info = _dexService->currentActivatedPair();

    auto convert = [](auto interval) {
        return QVariantMap{ { "from", static_cast<double>(interval.first) },
            { "to", static_cast<double>(interval.second) } };
    };

    result["buyFeePercent"] = info.buyFeePercent;
    result["sellFeePercent"] = info.sellFeePercent;

    result["buyFundsInterval"] = convert(info.buyFundsInterval);
    result["buyPriceInterval"] = convert(info.buyPriceInterval);

    result["sellFundsInterval"] = convert(info.sellFundsInterval);
    result["sellPriceInterval"] = convert(info.sellPriceInterval);

    return result;
}

//==============================================================================

AssetID WalletDexViewModel::quoteAssetID() const
{
    return hasSwapPair() ? _dexService->stateModel().quoteAssetID()
                         : std::numeric_limits<AssetID>::max();
}

//==============================================================================

AssetID WalletDexViewModel::baseAssetID() const
{
    return hasSwapPair() ? _dexService->stateModel().baseAssetID()
                         : std::numeric_limits<AssetID>::max();
}

//==============================================================================

bool WalletDexViewModel::isOnline() const
{
    return _dexService && _dexService->isConnected();
}

//==============================================================================

bool WalletDexViewModel::hasSwapPair() const
{
    return _dexService && _dexService->stateModel().hasSwapPair();
}

//==============================================================================

bool WalletDexViewModel::canPlaceOrder() const
{
    return _canPlaceOrder;
}

//==============================================================================

bool WalletDexViewModel::inMaintenance() const
{
    return _dexService && _dexService->isInMaintenance();
}

//==============================================================================

WalletDexStateModel* WalletDexViewModel::stateModel() const
{
    return _dexService ? &_dexService->stateModel() : nullptr;
}

//==============================================================================

void WalletDexViewModel::initialize(ApplicationViewModel* appViewModel)
{
    _walletAssetsModel = appViewModel->assetsModel();
    _dexService = appViewModel->dexService();

    if (_dexService->isStarted()) {
        onDexServiceReady();
    } else {
        connect(_dexService, &DexService::started, this, &WalletDexViewModel::onDexServiceReady);
    }

    stateModelChanged();
    swapAssetsChanged();
}

//==============================================================================
