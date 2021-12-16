#include "WalletDexChannelBalanceViewModel.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <LndTools/ConnextTypes.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

std::array<int64_t, 2> WalletDexChannelBalanceViewModel::convertConnextBalancesToSats(
    const std::array<eth::u256, 2> weiBalances, AssetID assetID)
{
    std::array<int64_t, 2> result;
    for (auto i = 0; i < weiBalances.size(); i++) {
        result[i] = eth::ConvertFromWeiToSats(weiBalances[i], UNITS_PER_CURRENCY.at(assetID), 8);
    }
    return result;
}

//==============================================================================

WalletDexChannelBalanceViewModel::WalletDexChannelBalanceViewModel(QObject* parent)
    : QObject(parent)
{
    _baseChannelsBalance = _quoteChannelsBalance = { 0, 0 };
}

//==============================================================================

AssetID WalletDexChannelBalanceViewModel::baseAssetID() const
{
    return _baseAssetID;
}

//==============================================================================

AssetID WalletDexChannelBalanceViewModel::quoteAssetID() const
{
    return _quoteAssetID;
}

//==============================================================================

QVariantMap WalletDexChannelBalanceViewModel::channelsBalance() const
{
    QVariantMap result;

    QVariantMap baseBalance;
    QVariantMap quoteBalance;

    baseBalance["localBalance"] = static_cast<double>(_baseChannelsBalance.at(0));
    baseBalance["remoteBalance"] = static_cast<double>(_baseChannelsBalance.at(1));

    quoteBalance["localBalance"] = static_cast<double>(_quoteChannelsBalance.at(0));
    quoteBalance["remoteBalance"] = static_cast<double>(_quoteChannelsBalance.at(1));

    result["baseBalance"] = baseBalance;
    result["quoteBalance"] = quoteBalance;

    return result;
}

//==============================================================================

void WalletDexChannelBalanceViewModel::initialize(ApplicationViewModel* applicationViewModel)
{
    _paymentNodesManager = applicationViewModel->paymentNodesManager();
    _assetsModel = applicationViewModel->assetsModel();
}

//==============================================================================

void WalletDexChannelBalanceViewModel::changeTraidingPair(AssetID baseAssetID, AssetID quoteAssetID)
{
    if (_baseAssetID == baseAssetID && _quoteAssetID == quoteAssetID) {
        return;
    }

    reset();

    _baseAssetID = baseAssetID;
    _quoteAssetID = quoteAssetID;

    tradingPairChanged();
    channelsBalanceChanged();

    initNodes();
}

//==============================================================================

void WalletDexChannelBalanceViewModel::reset()
{
    // very important, disconnect old interfaces from this receiver
    for (auto&& assetID : { _baseAssetID, _quoteAssetID }) {
        if (auto paymentNodeManager = _paymentNodesManager->interfaceById(assetID)) {
            disconnect(paymentNodeManager, nullptr, this, nullptr);
        }
    }

    _baseChannelsBalance = _quoteChannelsBalance = { 0, 0 };
}

//==============================================================================

void WalletDexChannelBalanceViewModel::initNodes()
{
    std::vector<std::pair<AssetID, PaymentNodeInterface*>> paymentNodes;
    paymentNodes.push_back({ _baseAssetID, _paymentNodesManager->interfaceById(_baseAssetID) });
    paymentNodes.push_back({ _quoteAssetID, _paymentNodesManager->interfaceById(_quoteAssetID) });

    if (std::all_of(paymentNodes.begin(), paymentNodes.end(),
            [](const auto& pair) { return pair.second; })) {
        for (auto&& pair : paymentNodes) {
            if (pair.second->type() == Enums::PaymentNodeType::Lnd) {
                initLndNode(pair.first);
            } else {
                initConnextNode(pair.first);
            }
        }
    }
}

//==============================================================================

void WalletDexChannelBalanceViewModel::initLndNode(AssetID assetID)
{
    auto onChannelsChanged = [this](std::array<int64_t, 2>& where, auto newChannels) {
        where = BuildChannelsBalance(newChannels);
        this->channelsBalanceChanged();
    };

    auto manager = qobject_cast<LnDaemonInterface*>(_paymentNodesManager->interfaceById(assetID));
    auto isNodeBase = assetID == _baseAssetID;

    connect(manager, &LnDaemonInterface::channelsChanged, this,
        std::bind(onChannelsChanged,
            std::ref(isNodeBase ? _baseChannelsBalance : _quoteChannelsBalance),
            std::placeholders::_1));

    manager->channels()
        .then([](std::vector<LndChannel> channels) { return BuildChannelsBalance(channels); })
        .then([this, isNodeBase](std::array<int64_t, 2> from) {
            isNodeBase ? _baseChannelsBalance.swap(from) : _quoteChannelsBalance.swap(from);
        });
}

//==============================================================================

void WalletDexChannelBalanceViewModel::initConnextNode(AssetID assetID)
{
    auto onChannelsChanged = [this, assetID](std::array<int64_t, 2>& where, auto newChannels) {
        where = convertConnextBalancesToSats(BuildConnextChannelsBalance(newChannels), assetID);
        this->channelsBalanceChanged();
    };

    auto manager
        = qobject_cast<ConnextDaemonInterface*>(_paymentNodesManager->interfaceById(assetID));

    auto isNodeBase = assetID == _baseAssetID;
    connect(manager, &ConnextDaemonInterface::channelsChanged, this,
        std::bind(onChannelsChanged,
            std::ref(isNodeBase ? _baseChannelsBalance : _quoteChannelsBalance),
            std::placeholders::_1));

    manager->channels()
        .then([this, assetID](std::vector<ConnextChannel> channels) {
            return convertConnextBalancesToSats(BuildConnextChannelsBalance(channels), assetID);
        })
        .then([this, isNodeBase](std::array<int64_t, 2> from) {
            isNodeBase ? _baseChannelsBalance.swap(from) : _quoteChannelsBalance.swap(from);
        });
}

//==============================================================================
