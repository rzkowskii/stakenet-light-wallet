#include "DexService.hpp"
#include <Data/TradingBotService.hpp>
#include <Data/TradingModelBatchedDataSource.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/DexSwapClientFactory.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Models/WalletDexStateModel.hpp>
#include <Orderbook/OrderbookClient.hpp>
#include <Orderbook/TradingOrdersModel.hpp>
#include <SwapService.hpp>
#include <Tools/AppConfig.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

DexService::DexService(const WalletAssetsModel& assetsModel,
    PaymentNodesManager& paymentNodesManager, AssetsTransactionsCache& txCache, QObject* parent)
    : QObject(parent)
    , _assetsModel(assetsModel)
    , _paymentNodesManager(paymentNodesManager)
    , _txCache(txCache)
    , _stateModel(new WalletDexStateModel(assetsModel, *this, this))
{
}

//==============================================================================

DexService::~DexService() {}

//==============================================================================

void DexService::start(QString clientPubKey)
{
    Q_ASSERT(!_swapService);
    init(clientPubKey);
    _swapService->start();
    started();
}

//==============================================================================

void DexService::stop()
{
    stopped();
}

//==============================================================================

bool DexService::isStarted() const
{
    return _swapService.operator bool();
}

//==============================================================================

bool DexService::isConnected() const
{
    return _isConnected;
}

//==============================================================================

bool DexService::isInMaintenance() const
{
    return _isInMaintenance;
}

//==============================================================================

bool DexService::IsStaging()
{
    return false;
}

//==============================================================================

TradingModelBatchedDataSource& DexService::ordersDataSource() const
{
    return *_batchedDataSource;
}

//==============================================================================

swaps::SwapService& DexService::swapService() const
{
    return *_swapService;
}

//==============================================================================

swaps::RefundableFeeManager* DexService::feeRefunding() const
{
    return _swapService->feeRefunding();
}

//==============================================================================

orderbook::OrderbookClient* DexService::orderbook() const
{
    return _swapService->orderBookClient();
}

//==============================================================================

orderbook::ChannelRentingManager* DexService::channelRentingManager() const
{
    return _swapService->channelRentingManager();
}

//==============================================================================

Promise<void> DexService::activatePair(QString pairId)
{
    auto currentActivatedPair = _currentActivatedPair;

    auto it = std::find_if(std::begin(_tradingPairs), std::end(_tradingPairs),
        [pairId](const auto& tp) { return tp.pairId == pairId; });

    if (it == std::end(_tradingPairs)) {
        LogCCritical(General) << "Trying to activate pair which was not found in traiding pairs"
                              << pairId;
        return Promise<void>::reject(std::runtime_error("Invalid traiding pair to activate"));
    }

    _currentActivatedPair = *it;

    return _swapService->deactivatePair(currentActivatedPair.pairId.toStdString())
        .then([this, pairId] {
            _swapService->activatePair(pairId.toStdString());
            return QtPromise::each(pairId.split("_").toVector(),
                [this](const QString& currency, ...) {
                    _swapService->addCurrency(currency.toStdString());
                })
                .then([] {});
        });
}

//==============================================================================

Promise<void> DexService::deactivateActivePair()
{
    decltype(_currentActivatedPair) currentActivatedPair;
    std::swap(_currentActivatedPair, currentActivatedPair);
    return _swapService->deactivatePair(currentActivatedPair.pairId.toStdString());
}

//==============================================================================

Promise<bool> DexService::verifyHubRoute(AssetID assetID, Balance amount, bool selfIsSource)
{
    auto nodesInterface = _paymentNodesManager.interfaceById(assetID);
    if (nodesInterface->type() == Enums::PaymentNodeType::Lnd) {
        auto lndManager = qobject_cast<LnDaemonInterface*>(nodesInterface);
        return lndManager->verifyHubRoute(amount, selfIsSource);
    } else {
        auto connextManager = qobject_cast<ConnextDaemonInterface*>(nodesInterface);
        if (assetID == 60002 || assetID == 60003 || assetID == 60) {
            return connextManager->verifyHubRoute(amount, selfIsSource);
        } else {
            return QtPromise::resolve(false);
        }
    }
}

//==============================================================================

Promise<void> DexService::addCurrencies(std::vector<AssetID> assetIDs)
{
    std::vector<std::string> currencies;
    std::transform(assetIDs.begin(), assetIDs.end(), std::back_inserter(currencies),
        [this](const auto& assetId) {
            return _assetsModel.assetById(assetId).ticket().toStdString();
        });

    return QtPromise::each(
        currencies, [this](const auto& currency, ...) { _swapService->addCurrency(currency); })
        .then([] {});
}

//==============================================================================

const DexService::TraidingPair& DexService::currentActivatedPair() const
{
    return _currentActivatedPair;
}

//==============================================================================

const DexService::TradingPairs& DexService::tradingPairs() const
{
    return _tradingPairs;
}

//==============================================================================

WalletDexStateModel& DexService::stateModel() const
{
    return *_stateModel;
}

//==============================================================================

TradingBotService& DexService::tradingBot() const
{
    return *_tradingBot;
}

//==============================================================================

void DexService::onStateChanged(swaps::SwapService::State state)
{
    _isConnected = state == swaps::SwapService::State::Connected;

    if (_isConnected) {
        _swapService->tradingPairs().then([this](std::vector<orderbook::TradingPair> pairs) {
            _tradingPairs.resize(pairs.size());
            std::transform(
                std::begin(pairs), std::end(pairs), std::begin(_tradingPairs), [](const auto& tp) {
                    TraidingPair result;
                    result.pairId = QString::fromStdString(tp.pairId);
                    result.buyFeePercent = tp.buyFeePercent;
                    result.sellFeePercent = tp.sellFeePercent;
                    result.buyFundsInterval = tp.buyFundsInterval;
                    result.buyPriceInterval = tp.buyPriceInterval;
                    result.sellFundsInterval = tp.sellFundsInterval;
                    result.sellPriceInterval = tp.sellPriceInterval;
                    return result;
                });
            // temporary removed  - not supported in wallet, but available in orderbook
            _tradingPairs.erase(std::remove_if(_tradingPairs.begin(), _tradingPairs.end(),
                                    [](const auto& pair) {
                                        return pair.pairId
                                            == ""
                                               "BTC_WETH";
                                    }),
                _tradingPairs.end());

            tradingPairsChanged();
        });
    }

    setInMaintenance(state == swaps::SwapService::State::InMaintenance);
    isConnectedChanged(_isConnected);
}

//==============================================================================

void DexService::onLightningBalanceChanged()
{
    auto activatedPairAssets = currentActivatedPair().pairId.split('_', QString::SkipEmptyParts);

    if (activatedPairAssets.isEmpty())
        return;

    for (auto&& asset : activatedPairAssets) {
        auto nodesManager
            = _paymentNodesManager.interfaceById(_assetsModel.assetByName(asset).coinID());

        nodesManager->refreshChannels();
    }
}

//==============================================================================

void DexService::init(QString clientPubKey)
{
    const auto& connextConfig = AppConfig::Instance().config().connextConfig;
    swaps::SwapService::Config cfg;
    cfg.dataDirPath = GetSwapRepositoryPath(ApplicationViewModel::IsEmulated());
    cfg.clientId = clientPubKey;
    cfg.connextConfig.host = connextConfig.resolverHost;
    cfg.connextConfig.port = connextConfig.resolverPort;

    _dexFactory
        = std::make_unique<DexSwapClientFactory>(_txCache, _paymentNodesManager, _assetsModel);
    cfg.swapClientFactory = _dexFactory.get();

    if (IsStaging()) {
        cfg.orderbookUrl = decltype(cfg)::StagingOrderbookUrl();
        cfg.payFee = true;
    } else {
        cfg.orderbookUrl = decltype(cfg)::DefaultOrderbookUrl();
    }

    _swapService.reset(new swaps::SwapService(cfg));
    connect(_swapService.get(), &swaps::SwapService::stateChanged, this,
        [this](swaps::SwapService::State newState) { onStateChanged(newState); });

    connect(_swapService.get(), &swaps::SwapService::swapExecuted, this,
        &DexService::onLightningBalanceChanged);

    _orderBatchingWorker.rename("Stakenet-DexBatching");
    _orderBatchingWorker.start();
    _batchedDataSource.reset(
        new TradingModelBatchedDataSource(_swapService->orderBookClient()->ordersModel()));
    _batchedDataSource->moveToThread(&_orderBatchingWorker);

    _dexWorker.rename("Stakenet-DexWorker");
    _dexWorker.start();
    _swapService->moveToThread(&_dexWorker);

    _tradingBot.reset(new TradingBotService(*this));
    _tradingBot->moveToThread(&_dexWorker);
}

//==============================================================================

void DexService::setInMaintenance(bool inMaintenance)
{
    if (_isInMaintenance != inMaintenance) {
        _isInMaintenance = inMaintenance;
        emit isInMaintenanceChanged(_isInMaintenance);
    }
}

//==============================================================================
