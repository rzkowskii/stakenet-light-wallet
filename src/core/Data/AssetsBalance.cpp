#include "AssetsBalance.hpp"

#include <Chain/Chain.hpp>
#include <Chain/ChainManager.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/AbstractAssetBalanceProviderFactory.hpp>
#include <ViewModels/LocalCurrencyViewModel.hpp>

#include <numeric>

//==============================================================================

AssetsBalance::AssetsBalance(QPointer<WalletAssetsModel> walletAssetsModel,
    QPointer<LocalCurrencyViewModel> localCurrencyModel,
    QPointer<AbstractChainManager> chainManager,
    std::unique_ptr<AbstractAssetBalanceProviderFactory> factory, QObject* parent)
    : QObject(parent)
    , _walletAssetsModel(walletAssetsModel)
    , _localCurrencyModel(localCurrencyModel)
    , _chainManager(chainManager)
    , _factory(std::move(factory))

{
    init();
}

//==============================================================================

AssetsBalance::~AssetsBalance() {}

//==============================================================================

auto AssetsBalance::balance() const -> const BalanceMap&
{
    return _assetsBalance;
}

//==============================================================================

auto AssetsBalance::localBalance() const -> const BalanceLocalMap&
{
    return _assetsLocalBalance;
}

//==============================================================================

Balance AssetsBalance::balanceSum() const
{
    return std::accumulate(std::begin(_assetsBalance), std::end(_assetsBalance), 0,
        [](Balance value, const auto& pair) { return value + pair.second.total(); });
}

//==============================================================================

double AssetsBalance::balanceLocalSum() const
{
    return std::accumulate(std::begin(_assetsLocalBalance), std::end(_assetsLocalBalance), 0.0,
        [](double value, const auto& pair) { return value + pair.second; });
}

//==============================================================================

bool AssetsBalance::hasBalance(AssetID assetID) const
{
    return _assetsBalance.count(assetID) > 0;
}

//==============================================================================

AssetBalance AssetsBalance::balanceById(AssetID assetID) const
{
    return _assetsBalance.at(assetID);
}

//==============================================================================

double AssetsBalance::balanceLocalById(AssetID assetID) const
{
    return _assetsLocalBalance.at(assetID);
}

//==============================================================================

void AssetsBalance::update()
{
    for (const auto& it : _providers) {
        it.second->update();
    }
}

//==============================================================================

void AssetsBalance::onLocalCurrencyChanged()
{
    for (auto& it : _assetsLocalBalance) {
        onCurrencyRateChanged(it.first);
    }
}

//==============================================================================

void AssetsBalance::onCurrencyRateChanged(AssetID assetID)
{
    if (_assetsBalance.count(assetID) > 0) {
        _assetsLocalBalance[assetID]
            = _localCurrencyModel->convertSats(assetID, _assetsBalance.at(assetID).total())
            / static_cast<double>(COIN);

        localBalanceChanged(assetID);
    }
}

//==============================================================================

void AssetsBalance::updateBalance(AssetID assetID, AssetBalance balance)
{
    _assetsBalance[assetID] = balance;

    auto it = _assetsLocalBalance.find(assetID);
    if (it != _assetsLocalBalance.end()) {
        it->second = _localCurrencyModel->convertSats(assetID, balance.total())
            / static_cast<double>(COIN);
    } else {
        _assetsLocalBalance[assetID] = _localCurrencyModel->convertSats(assetID, balance.balance)
            / static_cast<double>(COIN);
    }

    emit balanceChanged(assetID);
    emit confirmedBalanceChanged(assetID);

    emit localBalanceChanged(assetID);
}

//==============================================================================

void AssetsBalance::init()
{
    connect(_localCurrencyModel, &LocalCurrencyViewModel::localCurrencyChanged, this,
        &AssetsBalance::onLocalCurrencyChanged);
    connect(_localCurrencyModel, &LocalCurrencyViewModel::currencyRateChanged, this,
        &AssetsBalance::onCurrencyRateChanged);

    connect(_chainManager, &AbstractChainManager::chainsLoaded, this,
        [this](std::vector<AssetID> assets) {
            for (auto&& assetID : assets) {
                if (_providers.count(assetID) == 0) {
                    this->createProvider(assetID);
                }
            }
        });
}

//==============================================================================

void AssetsBalance::createProvider(AssetID assetID)
{
    auto provider = _factory->createBalanceProvider(_walletAssetsModel->assetById(assetID));
    auto providerPtr = provider.get();
    _providers[assetID] = std::move(provider);
    connect(providerPtr, &AbstractAssetBalanceProvider::balanceChanged, this,
        std::bind(&AssetsBalance::updateBalance, this, assetID, std::placeholders::_1));
}

//==============================================================================
