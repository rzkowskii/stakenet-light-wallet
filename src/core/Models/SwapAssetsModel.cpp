#include "SwapAssetsModel.hpp"
#include <Data/CoinAsset.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Models/DexService.hpp>
#include <Models/DexStatusManager.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>

using namespace boost::adaptors;

static const QString SETTINGS_LAST_BASE_ASSETID("lastBaseAssetID");
static const QString SETTINGS_LAST_QUOTE_ASSETID("lastQuoteAssetID");

//==============================================================================

SwapAssetsModel::SwapAssetsModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

//==============================================================================

SwapAssetsModel::~SwapAssetsModel() {}

//==============================================================================

int SwapAssetsModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(_impl.size());
}

//==============================================================================

QVariant SwapAssetsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex()) || _assetModel.isNull()) {
        return QVariant();
    }
    const auto& swapAssets = _impl.at(static_cast<size_t>(index.row()));
    auto baseSymbol = _assetModel->assetById(swapAssets.first).ticket();
    auto quoteSymbol = _assetModel->assetById(swapAssets.second).ticket();

    switch (role) {
    case BaseAssetID:
        return swapAssets.first;
    case QuoteAssetID:
        return swapAssets.second;
    case BaseAssetSymbol:
        return baseSymbol;
    case QuoteAssetSymbol:
        return quoteSymbol;
    case SwapPairFilterRole:
        return QString("%1/%2").arg(baseSymbol).arg(quoteSymbol);
    case DexPairStatusRole:
        return static_cast<int>(getDexStatus(swapAssets.first, swapAssets.second));
    default:
        break;
    }
    return QVariant();
}
//==============================================================================

QHash<int, QByteArray> SwapAssetsModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[BaseAssetID] = "baseAssetID";
        roles[QuoteAssetID] = "quoteAssetID";
        roles[BaseAssetSymbol] = "baseAssetSymbol";
        roles[QuoteAssetSymbol] = "quoteAssetSymbol";
        roles[SwapPairFilterRole] = "swapPair";
        roles[DexPairStatusRole] = "dexStatus";
    }

    return roles;
}

//==============================================================================

void SwapAssetsModel::initialize(QObject* appViewModel)
{
    if (auto viewModel = qobject_cast<ApplicationViewModel*>(appViewModel)) {
        _assetModel = viewModel->assetsModel();
        _dexService = viewModel->dexService();
        _dexStatusManager = viewModel->dexStatusManager();
        connect(_dexService, &DexService::tradingPairsChanged, this,
            &SwapAssetsModel::onTradingPairsChanged);
        connect(_dexStatusManager, &DexStatusManager::dexStatusChanged, this,
            &SwapAssetsModel::onDexStatusChanged);
        onTradingPairsChanged();
    }
}

//==============================================================================

QVariantMap SwapAssetsModel::getByPair(QString swapPair)
{
    auto vecSymbols = swapPair.split("/");
    auto assetPair = DexAssetPair(_assetModel->assetByName(vecSymbols[0]).coinID(),
        _assetModel->assetByName(vecSymbols[1]).coinID());
    auto it = std::find(_impl.begin(), _impl.end(), assetPair);
    if (it != _impl.end()) {
        return get(static_cast<int>(std::distance(_impl.begin(), it)));
    }
    return QVariantMap();
}

//==============================================================================

QVariantMap SwapAssetsModel::get(int row)
{
    QHash<int, QByteArray> names = roleNames();
    QHashIterator<int, QByteArray> it(names);
    QModelIndex idx = index(row);
    QVariantMap result;
    while (it.hasNext()) {
        it.next();
        QVariant data = idx.data(it.key());
        result[it.value()] = data;
    }
    return result;
}

//==============================================================================

int SwapAssetsModel::lastTradingPairIndex()
{
    QSettings settings;
    auto baseAssetID = settings.value(SETTINGS_LAST_BASE_ASSETID).toUInt();
    auto quoteAssetID = settings.value(SETTINGS_LAST_QUOTE_ASSETID).toUInt();

    auto it = find(_impl.begin(), _impl.end(), DexAssetPair(baseAssetID, quoteAssetID));
    if (it != std::end(_impl)) {
        return static_cast<int>(std::distance(_impl.begin(), it));
    }
    return -1;
}

//==============================================================================

int SwapAssetsModel::lastBaseAssetID() const
{
    QSettings settings;
    if (settings.contains(SETTINGS_LAST_BASE_ASSETID)) {
        return settings.value(SETTINGS_LAST_BASE_ASSETID).toUInt();
    }
    return -1;
}

//==============================================================================

int SwapAssetsModel::lastQuoteAssetID() const
{
    QSettings settings;
    if (settings.contains(SETTINGS_LAST_QUOTE_ASSETID)) {
        return settings.value(SETTINGS_LAST_QUOTE_ASSETID).toUInt();
    }
    return -1;
}

//==============================================================================

std::vector<QString> SwapAssetsModel::baseAssets() const
{
    std::vector<QString> result;
    for (auto pair : _impl) {
        result.push_back(_assetModel->assetById(pair.first).ticket());
    }
    //TODO: remove after pairs will work
    // temporarily to disable on SSUI
    result.erase(std::remove( result.begin(), result.end(), "BTC"),
        result.end());
    return result;
}

//==============================================================================

std::vector<QString> SwapAssetsModel::quoteAssets() const
{
    std::vector<QString> result;
    for (auto pair : _impl) {
        result.push_back(_assetModel->assetById(pair.second).ticket());
    }
    //TODO: remove after pairs will work
    // temporarily to disable on SSUI
    result.erase(std::remove(result.begin(), result.end(), "USDT"),
        result.end());
    result.erase(std::remove(result.begin(), result.end(), "USDC"),
        result.end());
    return result;
}

//==============================================================================

bool SwapAssetsModel::hasPair(AssetID baseAssetID, AssetID quoteAssetID)
{
    auto it
        = std::find_if(_impl.begin(), _impl.end(), [baseAssetID, quoteAssetID](const auto& pair) {
              return pair.first == baseAssetID && pair.second == quoteAssetID;
          });

    return it != _impl.end();
}

//==============================================================================

void SwapAssetsModel::onTradingPairsChanged()
{
    beginResetModel();
    _impl.clear();
    const auto& pairs = _dexService->tradingPairs();

    decltype(_impl) temp(pairs.size());
    try {
        auto transform = [this](const auto& tp) {
            auto assets = tp.pairId.split("_");

            return DexAssetPair{ _assetModel->assetByName(assets.at(0)).coinID(),
                _assetModel->assetByName(assets.at(1)).coinID() };
        };
        auto filter = [this](const auto& tp) {
            auto assets = tp.pairId.split("_");
            return _assetModel->hasAsset(assets.at(0)) && _assetModel->hasAsset(assets.at(1));
        };

        boost::copy(pairs | filtered(filter) | transformed(transform), std::back_inserter(temp));

    } catch (std::exception& ex) {
        LogCCritical(General) << "Failed to parse assets" << ex.what();
        temp.clear();
    }

    // XSN_BTC, LTC_BTC, BTC_ETH, BTC_USDT, BTC_USDC, ETH_USDC
    std::set<DexAssetPair> whitelist{ { 384, 0 }, { 2, 0 }, { 60, 0 },
                                      { 0, 60002 }, { 0, 60003 },{ 60, 60003 } };

    if (DexService::IsStaging()) {
        whitelist.emplace(384, 2); // XSN_LTC
        // whitelist.emplace(384, 60001); // XSN_WETH
    }

    for (auto&& t : temp) {
        if (whitelist.count(t)) {
            _impl.push_back(t);
        }
    }

    endResetModel();
    tradingPairsChanged();
}

//==============================================================================

void SwapAssetsModel::onDexStatusChanged(AssetID assetID)
{
    auto it = std::find_if(_impl.begin(), _impl.end(),
        [assetID](const auto& pair) { return pair.first == assetID || pair.second == assetID; });

    if (it != _impl.end()) {
        auto modelIndex = index(std::distance(std::begin(_impl), it));
        dataChanged(modelIndex, modelIndex, { DexPairStatusRole });
        dexStatusChanged();
    }
}

//==============================================================================

Enums::DexTradingStatus SwapAssetsModel::getDexStatus(AssetID baseID, AssetID quoteID) const
{
    if (_dexStatusManager) {
        auto baseDexStatus = _dexStatusManager->dexStatusById(baseID);
        auto quoteDexStatus = _dexStatusManager->dexStatusById(quoteID);

        if (baseDexStatus == Enums::DexTradingStatus::Online
            && quoteDexStatus == Enums::DexTradingStatus::Online) {
            return Enums::DexTradingStatus::Online;
        } else if ((baseDexStatus == Enums::DexTradingStatus::Syncing
                       && quoteDexStatus == Enums::DexTradingStatus::Syncing)
            || (baseDexStatus == Enums::DexTradingStatus::Syncing
                   && quoteDexStatus == Enums::DexTradingStatus::Online)
            || (baseDexStatus == Enums::DexTradingStatus::Online
                   && quoteDexStatus == Enums::DexTradingStatus::Syncing)) {
            return Enums::DexTradingStatus::Syncing;
        } else {
            return Enums::DexTradingStatus::Offline;
        }
    }

    return Enums::DexTradingStatus::Offline;
}

//==============================================================================
