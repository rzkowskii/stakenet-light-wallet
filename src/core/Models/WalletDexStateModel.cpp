#include "WalletDexStateModel.hpp"

#include <Data/AllOrdersDataSource.hpp>
#include <Data/OrderBookData.hpp>
#include <Data/OwnOrdersDataSource.hpp>
#include <Data/TradingModelBatchedDataSource.hpp>
#include <Data/TradingModelBatchedProxy.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Models/AskBidListModel.hpp>
#include <Models/DexService.hpp>
#include <Models/OwnOrdersHistoryListModel.hpp>
#include <Models/OwnOrdersListModel.hpp>
#include <Models/TradeHistoryListModel.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/TradingOrdersModel.hpp>
#include <SwapService.hpp>
#include <Utils/Logging.hpp>

//==============================================================================

QString SwapFailureMsg(swaps::SwapFailureReason swapReason)
{
    switch (swapReason) {
    case (swaps::SwapFailureReason::OrderNotFound):
        return "Could not find the order specified by a swap request.";
    case (swaps::SwapFailureReason::OrderOnHold):
        return "The order specified by a swap request is on hold for a different ongoing swap.";
    case (swaps::SwapFailureReason::InvalidSwapRequest):
        return "The swap request contained invalid data.";
    case (swaps::SwapFailureReason::SwapClientNotSetup):
        return "You are not connected to both swap clients, or you are missing pub key identifiers "
               "for the peer's nodes.";
    case (swaps::SwapFailureReason::NoRouteFound):
        return "The LN network graph is waiting for updates, please wait for it to complete before "
               "placing an order. This may take up to 15 minutes.";
    case (swaps::SwapFailureReason::UnexpectedClientError):
        return "A swap client call failed for an unexpected reason.";
    case (swaps::SwapFailureReason::InvalidSwapPacketReceived):
        return "Received a swap packet from the peer with invalid data.";
    case (swaps::SwapFailureReason::SendPaymentFailure):
        return "The call to send payment failed.";
    case (swaps::SwapFailureReason::InvalidResolveRequest):
        return "The swap resolver request was invalid.";
    case (swaps::SwapFailureReason::PaymentHashReuse):
        return "The swap request attempts to reuse a payment hash.";
    case (swaps::SwapFailureReason::SwapTimedOut):
        return "The swap timed out while we were waiting for it to complete execution.";
    case (swaps::SwapFailureReason::DealTimedOut):
        return "The deal timed out while we were waiting for the peer to respond to our swap "
               "request.";
    case (swaps::SwapFailureReason::UnknownError):
        return "The swap failed due to an unrecognized error.";
    case (swaps::SwapFailureReason::RemoteError):
        return "The swap failed due to an error or unexpected behavior on behalf of the remote "
               "peer.";
    case (swaps::SwapFailureReason::SwapToYourself):
        return "Tried to execute swap to ourself";

    default:
        return { "Unknown reason" };
    }
}

//==============================================================================

QString ErrorMessageFromSwapFailure(const swaps::SwapFailure& failure)
{
    if (failure.failureReason == swaps::SwapFailureReason::SendPaymentFailure) {
        return QString::fromStdString(failure.failureMessage);
    }

    return SwapFailureMsg(failure.failureReason);
}

//==============================================================================

static const QString SETTINGS_LAST_BASE_ASSETID("lastBaseAssetID");
static const QString SETTINGS_LAST_QUOTE_ASSETID("lastQuoteAssetID");

//==============================================================================

WalletDexStateModel::WalletDexStateModel(
    const WalletAssetsModel& assetsModel, DexService& dexService, QObject* parent)
    : QObject(parent)
    , _walletAssetsModel(assetsModel)
    , _dexService(dexService)
{
    connect(
        &_dexService, &DexService::lndsChanged, this, &WalletDexStateModel::setCurrentSwapAssets);
}

//==============================================================================

TradeHistoryListModel* WalletDexStateModel::tradeHistoryListModel()
{
    return hasSwapPair() ? _tradeHistoryListModels.at(*_currentSwapAssets).get() : nullptr;
}

//==============================================================================

OrderBookListModel* WalletDexStateModel::sellOrdersListModel()
{
    return hasSwapPair() ? _sellOrdersListModels.at(*_currentSwapAssets).get() : nullptr;
}

//==============================================================================

OrderBookListModel* WalletDexStateModel::buyOrdersListModel()
{
    return hasSwapPair() ? _buyOrdersListModels.at(*_currentSwapAssets).get() : nullptr;
}

//==============================================================================

OwnOrdersListModel* WalletDexStateModel::ownOrderBookListModel()
{
    return _ownOrdersModel.get();
}

//==============================================================================

OwnOrdersHistoryListModel* WalletDexStateModel::ownOrdersHistoryListModel()
{
    return hasSwapPair() ? _ownOrdersHistoryListModel.at(*_currentSwapAssets).get() : nullptr;
}

//==============================================================================

bool WalletDexStateModel::hasSwapPair() const
{
    return _currentSwapAssets.has_value();
}

//==============================================================================

void WalletDexStateModel::setCurrentSwapAssets(
    std::optional<WalletDexStateModel::SwapAssets> swapAssets)
{
    auto current = _currentSwapAssets.value_or(SwapAssets{ 0, 0 });
    auto newAssets = swapAssets.value_or(SwapAssets{ 0, 0 });
    if (current.first != newAssets.first || current.second != newAssets.second) {
        _currentSwapAssets = swapAssets;
        if (hasSwapPair()) {
            writeSettings();
            updateCurrentModels();
        }
        swapAssetsChanged();
    }
}

//==============================================================================

void WalletDexStateModel::updateCurrentModels()
{
    auto tradingModel = _dexService.orderbook()->ordersModel();
    auto batchedDataSource = &_dexService.ordersDataSource();
    auto pairId = getPairId();

    qobject_delete_later_unique_ptr<TradingModelBatchedProxy> sellDataSourceProxy(
        new TradingModelBatchedProxy(
            batchedDataSource, pairId, AllOrdersDataSource::OrdersType::Sell));
    qobject_delete_later_unique_ptr<TradingModelBatchedProxy> buyDataSourceProxy(
        new TradingModelBatchedProxy(
            batchedDataSource, pairId, AllOrdersDataSource::OrdersType::Buy));

    sellDataSourceProxy->moveToThread(batchedDataSource->thread());
    buyDataSourceProxy->moveToThread(batchedDataSource->thread());

    auto currentSwapAssets = *_currentSwapAssets;

    _sellOrdersDataSource[currentSwapAssets]
        = std::make_unique<AllOrdersDataSource>(std::move(sellDataSourceProxy), pairId);
    _buyOrdersDataSource[currentSwapAssets]
        = std::make_unique<AllOrdersDataSource>(std::move(buyDataSourceProxy), pairId);

    //    if (!_ownOrdersDataSource || _ownOrdersDataSource.get()->pairId() != pairId) {
    _ownOrdersDataSource = std::make_unique<OwnOrdersDataSource>(_dexService.orderbook(), pairId);
    _ownOrdersModel = std::make_unique<OwnOrdersListModel>(_ownOrdersDataSource.get(), pairId);
    //    }

    _sellOrdersListModels[currentSwapAssets] = std::make_unique<AskBidListModel>(
        _sellOrdersDataSource.at(currentSwapAssets).get(), _ownOrdersDataSource.get(), pairId);
    _buyOrdersListModels[currentSwapAssets] = std::make_unique<AskBidListModel>(
        _buyOrdersDataSource.at(currentSwapAssets).get(), _ownOrdersDataSource.get(), pairId);

    _sellOrdersDataSource[currentSwapAssets]->fetch();
    _buyOrdersDataSource[currentSwapAssets]->fetch();

    _tradeHistoryListModels[currentSwapAssets]
        = std::make_unique<TradeHistoryListModel>(tradingModel, pairId);
    _ownOrdersHistoryListModel[currentSwapAssets] = std::make_unique<OwnOrdersHistoryListModel>(
        _dexService.swapService().swapRepository(), _dexService.feeRefunding(), this);
}

//==============================================================================

QString WalletDexStateModel::getPairId() const
{
    if (hasSwapPair()) {
        auto baseSymbol
            = _walletAssetsModel.assetById(_currentSwapAssets->first).ticket().toUpper();
        auto quoteSymbol
            = _walletAssetsModel.assetById(_currentSwapAssets->second).ticket().toUpper();
        return QString("%1_%2").arg(baseSymbol).arg(quoteSymbol);
    }

    return QString();
}

//==============================================================================

AssetID WalletDexStateModel::quoteAssetID() const
{
    return _currentSwapAssets
        .value_or(
            SwapAssets{ std::numeric_limits<AssetID>::max(), std::numeric_limits<AssetID>::max() })
        .second;
}

//==============================================================================

AssetID WalletDexStateModel::baseAssetID() const
{
    return _currentSwapAssets
        .value_or(
            SwapAssets{ std::numeric_limits<AssetID>::max(), std::numeric_limits<AssetID>::max() })
        .first;
}

//==============================================================================

Promise<void> WalletDexStateModel::createOrder(
    Balance baseQuantity, Balance quoteQuantity, double price, bool isBuy)
{
    // pair XSN/LTC, base = XSN, quote = LTC
    // amount is always in base currency
    // total is always in quote currency

    AssetID baseCurrency = baseAssetID();
    AssetID quoteCurrency = quoteAssetID();

    const bool isLimit = price > 0;
    // if we are buying then we own quote currency, if selling base currency
    const auto quantity = isBuy ? quoteQuantity : baseQuantity;
    orderbook::LimitOrder limitOrder(
        orderbook::MarketOrder{ quantity, getPairId().toStdString(), isBuy },
        isLimit ? static_cast<int64_t>(price) : 0);

    LogCDebug(Swaps) << "Checking routes:" << getPairId() << Qt::endl
                     << "Base currency:" << baseCurrency << "amount:" << baseQuantity
                     << "fromUsToHub:" << !isBuy << Qt::endl
                     << "Quote currency:" << quoteCurrency << "amount:" << quoteQuantity
                     << "fromUsToHub:" << isBuy;

    std::vector<std::tuple<AssetID, Balance, bool>> values
        = { std::make_tuple(baseCurrency, 100 /*baseQuantity*/, !isBuy),
              std::make_tuple(quoteCurrency, 100 /*quoteQuantity*/, isBuy) };

    auto cur = values.front();

    return _dexService.verifyHubRoute(std::get<0>(cur), std::get<1>(cur), std::get<2>(cur))
        .then([this, baseAsset = std::get<0>(cur), cur = values.back()](bool hasRoute) {
            if (!hasRoute) {
                noHubRouteError(baseAsset);
                return QtPromise::resolve(false);
            }

            return _dexService.verifyHubRoute(std::get<0>(cur), std::get<1>(cur), std::get<2>(cur))
                .then([this, quoteAsset = std::get<0>(cur)](bool hasRoute) {
                    if (!hasRoute) {
                        noHubRouteError(quoteAsset);
                        return false;
                    }

                    return true;
                })
                .fail([this, quoteAsset = std::get<0>(cur)] {
                    noHubRouteError(quoteAsset);
                    return false;
                });
        })
        .fail([this, baseAsset = std::get<0>(cur)] {
            noHubRouteError(baseAsset);
            return false;
        })
        .then([this, limitOrder](bool hasRoutes) {
            if (!hasRoutes) {
                return QtPromise::resolve();
            }

            return _dexService.swapService()
                .placeOrder(limitOrder)
                .then([](swaps::PlaceOrderResult result) {
                    if (auto ownOrder = boost::get<orderbook::OwnOrder>(&result)) {
                        LogCDebug(Swaps) << "Place order finished!";
                        LogCDebug(Swaps)
                            << "Pair id : " << QString::fromStdString(ownOrder->pairId);
                        LogCDebug(Swaps) << "Quantity :" << ownOrder->quantity;

                    } else if (auto success = boost::get<swaps::SwapSuccess>(&result)) {
                        LogCDebug(Swaps)
                            << "Place order finished with swap:" << success->rHash.c_str();
                        LogCDebug(Swaps) << "Pair id:" << success->pairId.c_str();
                        LogCDebug(Swaps) << "Quantity:" << success->quantity;
                    }
                })
                .fail([](swaps::PlaceOrderFailure failure) {
                    QString errorMessage("Place order failed!");
                    if (auto placeOrderFailure = boost::get<swaps::SwapFailure>(&failure)) {
                        LogCCritical(Swaps)
                            << "Place order failed with failure:"
                            << QString::fromStdString(placeOrderFailure->failureMessage);

                        errorMessage = ErrorMessageFromSwapFailure(*placeOrderFailure);
                    } else if (auto feeFailure = boost::get<swaps::FeeFailure>(&failure)) {
                        auto reason = [reason = feeFailure->reason] {
                            using swaps::FeeFailure;
                            switch (reason) {
                            case FeeFailure::Reason::RemoteError:
                                return "remote error";
                            case FeeFailure::Reason::NoRouteFound:
                                return "no route found";
                            case FeeFailure::Reason::UnknownReason:
                                return "unknown";
                            case FeeFailure::Reason::NotEnoughBalance:
                                return "not enough balance";
                            }

                            return "";
                        }();
                        errorMessage = QString("Failed to pay fee, reason: %1").arg(reason);
                    }

                    throw std::runtime_error(errorMessage.toStdString());
                })
                .fail([](const std::exception& error) {
                    LogCCritical(Swaps) << "Place order failed with message:" << error.what();
                    auto errorMessage
                        = QString("Place order failed with error: %1").arg(error.what());

                    return Promise<void>::reject(std::runtime_error(errorMessage.toStdString()));
                });
            //                .tapFail([] {
            //                    LogCCritical(Swaps) << "Place order failed with unknown error";
            //                    throw std::runtime_error("Place order failed with unknown error");
            //                });
        });
}

//==============================================================================

void WalletDexStateModel::writeSettings()
{
    if (_currentSwapAssets) {
        QSettings settings;
        settings.setValue(
            SETTINGS_LAST_BASE_ASSETID, QVariant::fromValue(_currentSwapAssets->first));
        settings.setValue(
            SETTINGS_LAST_QUOTE_ASSETID, QVariant::fromValue(_currentSwapAssets->second));
        settings.sync();
    }
}

//==============================================================================
