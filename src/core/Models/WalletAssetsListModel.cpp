#include "WalletAssetsListModel.hpp"
#include <Chain/AbstractChainDataSource.hpp>
#include <Data/AssetsBalance.hpp>
#include <Data/CoinAsset.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Models/DexStatusManager.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>
#include <ViewModels/LocalCurrencyViewModel.hpp>
#include <numeric>

//==============================================================================

struct WalletAssetsListModel::WalletAssetsListModelImpl {
    std::vector<CoinAsset> _assets;
    AssetsBalance::BalanceMap _balance;
    Balance _balanceSum;
    double _balanceLocalSum;

    void updateCache(AssetsBalance* balance)
    {
        _balance = balance->balance();
        _balanceSum = balance->balanceSum();
        _balanceLocalSum = balance->balanceLocalSum();
    }
};

//==============================================================================

WalletAssetsListModel::WalletAssetsListModel(QObject* parent)
    : QAbstractListModel(parent)
    , _impl(new WalletAssetsListModelImpl)
{
}

//==============================================================================

WalletAssetsListModel::~WalletAssetsListModel() {}

//==============================================================================

int WalletAssetsListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return static_cast<int>(_impl->_assets.size());
}

//==============================================================================

QVariant WalletAssetsListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }
    const auto& asset = _impl->_assets.at(static_cast<size_t>(index.row()));
    Balance balanceTotal = getAssetTotalBalance(asset.coinID());
    Balance balanceOnChain = getAssetBalanceOnChain(asset.coinID());
    Balance balanceNode = getAssetNodeBalance(asset.coinID());
    Balance confirmedBalanceOnChain = getConfirmedAssetBalanceOnChain(asset.coinID());
    auto savedSatsPerByte = getSatsPerByte(asset.coinID());
    auto satsPerByte = savedSatsPerByte > 0 ? savedSatsPerByte : asset.lndData().defaultSatsPerByte;

    switch (role) {
    case IDRole:
        return asset.coinID();
    case NameRole:
        return asset.name();
    case TicketRole:
        return asset.ticket();
    case ColorRole:
        return asset.misc().color;
    case BalanceRole:
        return static_cast<double>(balanceTotal);
    case PortfolioPercentageRole:
        return getPortfolioLocalPercentage(balanceTotal, asset.coinID());
    case DescriptionRole:
        return asset.misc().coinDescription;
    case IsActiveRole:
        return isAssetActive(asset.coinID());
    case OfficialLinkRole:
        return asset.misc().officialLink;
    case RedditLinkRole:
        return asset.misc().redditLink;
    case TwitterLinkRole:
        return asset.misc().twitterLink;
    case TelegramLinkRole:
        return asset.misc().telegramLink;
    case IsAlwaysActiveRole:
        return asset.misc().isAlwaysActive;
    case SymbolNameFilterRole:
        return QString(asset.name() + asset.ticket());
    case ConfirmationsForApproved:
        return asset.misc().confirmationsForApproved;
    case BalanceOnChainRole:
        return static_cast<double>(balanceOnChain);
    case ConfirmedBalanceOnChainRole:
        return static_cast<double>(confirmedBalanceOnChain);
    case NodeBalanceRole:
        return static_cast<double>(balanceNode);
    case AverageSycBlockForSecRole:
        return asset.misc().averageSycBlockForSec;
    case MinLndCapacityRole:
        return asset.lndData().minChannelCapacity;
    case MaxLndCapacityRole:
        return asset.lndData().maxChannelCapacity;
    case MinPaymentAmountRole:
        return asset.lndData().minPaymentAmount;
    case MaxPaymentAmountRole:
        return asset.lndData().maxPaymentAmount;
    case SatsPerByteRole:
        return satsPerByte;
    case ConfirmationsForChannelApproved:
        return asset.lndData().confirmationForChannelApproved;
    case AvailableNodeBalanceRole:
        return static_cast<double>(getAssetAvailableNodeBalance(asset.coinID()));
    case RentalChannelsPerCoinRole:
        return asset.lndData().rentalChannelsPerCoin;
    case DexStatusRole:
        return static_cast<int>(getDexStatus(asset.coinID()));
    case ActiveNodeBalanceRole:
        return static_cast<double>(getActiveNodeBalance(asset.coinID()));
    case AverageTxFeeRole:
        return static_cast<double>(asset.misc().averageTxSize * satsPerByte);
    case ChainIDRole:
        return asset.token() ? asset.params().params.extCoinType() : asset.coinID();
    default:
        break;
    }

    return QVariant();
}

//==============================================================================

QHash<int, QByteArray> WalletAssetsListModel::roleNames() const
{
    static QHash<int, QByteArray> roles;

    if (roles.empty()) {
        roles[IDRole] = "id";
        roles[NameRole] = "name";
        roles[TicketRole] = "symbol";
        roles[ColorRole] = "color";
        roles[BalanceRole] = "balance";
        roles[PortfolioPercentageRole] = "percent";
        roles[DescriptionRole] = "description";
        roles[IsActiveRole] = "isActive";
        roles[OfficialLinkRole] = "officialLink";
        roles[RedditLinkRole] = "redditLink";
        roles[TwitterLinkRole] = "twitterLink";
        roles[TelegramLinkRole] = "telegramLink";
        roles[IsAlwaysActiveRole] = "isAlwaysActive";
        roles[SymbolNameFilterRole] = "nameSymbol";
        roles[ConfirmationsForApproved] = "confirmationsForApproved";
        roles[BalanceOnChainRole] = "balanceOnChain";
        roles[NodeBalanceRole] = "nodeBalance";
        roles[MinLndCapacityRole] = "minLndCapacity";
        roles[MaxLndCapacityRole] = "maxLndCapacity";
        roles[AverageSycBlockForSecRole] = "averageSycBlockForSec";
        roles[MinPaymentAmountRole] = "minPaymentAmount";
        roles[MaxPaymentAmountRole] = "maxPaymentAmount";
        roles[SatsPerByteRole] = "satsPerByte";
        roles[ConfirmationsForChannelApproved] = "confirmationsForChannelApproved";
        roles[AvailableNodeBalanceRole] = "availableLndBalance";
        roles[RentalChannelsPerCoinRole] = "rentalChannelsPerCoin";
        roles[DexStatusRole] = "dexStatus";
        roles[ActiveNodeBalanceRole] = "activeLndBalance";
        roles[AverageTxFeeRole] = "averageTxFee";
        roles[ConfirmedBalanceOnChainRole] = "confirmedBalanceOnChain";
        roles[ChainIDRole] = "chainId";
    }

    return roles;
}

//==============================================================================

QVariantMap WalletAssetsListModel::get(int row)
{
    QHash<int, QByteArray> names = roleNames();
    QHashIterator<int, QByteArray> it(names);
    QVariantMap result;
    while (it.hasNext()) {
        it.next();
        QModelIndex idx = index(row);
        QVariant data = idx.data(it.key());
        result[it.value()] = data;
    }
    return result;
}

//==============================================================================

long WalletAssetsListModel::getInitial(AssetID assetID)
{
    auto it = find_if(_impl->_assets.begin(), _impl->_assets.end(),
        [&assetID](CoinAsset asset) { return asset.coinID() == assetID; });
    return std::distance(_impl->_assets.begin(), it);
}

//==============================================================================

void WalletAssetsListModel::toogleAssetActiveState(AssetID assetID)
{
    const auto& asset = _impl->_assets.at(getInitial(assetID));
    auto settings = _assetModel->assetSettings(asset.coinID());
    settings->setIsActive(!settings->isActive());
}

//==============================================================================

void WalletAssetsListModel::changeAssetSatsPerByte(AssetID assetID, unsigned newValue)
{
    const auto& asset = _impl->_assets.at(getInitial(assetID));
    auto settings = _assetModel->assetSettings(asset.coinID());
    settings->setSatsPerByte(newValue);
}

//==============================================================================

void WalletAssetsListModel::averageFeeForAsset(AssetID assetID)
{
    const auto& asset = _impl->_assets.at(getInitial(assetID));
    QPointer<WalletAssetsListModel> self(this);
    _chainDataSource->estimateNetworkFee(assetID, asset.lndData().blocksForEstimateFee)
        .then([self, assetID](int64_t estimateFee) {
            if (!self) {
                return;
            }
            self->averageFeeForAssetFinished(
                assetID, static_cast<double>(ConvertToSatsPerByte(estimateFee)));
        })
        .fail([self](const std::exception& ex) {
            LogCDebug(Api) << "Failed to get raw transaction" << ex.what();
        });
}

//==============================================================================

void WalletAssetsListModel::onAssetIsActiveChanged(AssetID assetID)
{
    auto it = std::find_if(std::begin(_impl->_assets), std::end(_impl->_assets),
        [assetID](const CoinAsset& asset) { return asset.coinID() == assetID; });
    int row = std::distance(std::begin(_impl->_assets), it);
    dataChanged(index(row), index(row));
    accountBalanceChanged();
    activeAssetsCountChanged();
}

//==============================================================================

int WalletAssetsListModel::count() const
{
    return static_cast<int>(_impl->_assets.size());
}

//==============================================================================

int WalletAssetsListModel::activeAssetsCount() const
{
    if (_assetModel) {
        return static_cast<int>(_assetModel->activeAssets().size());
    }

    return 0;
}

//==============================================================================

void WalletAssetsListModel::initialize(QObject* appViewModel)
{
    if (auto viewModel = qobject_cast<ApplicationViewModel*>(appViewModel)) {
        _localCurrencyModel = viewModel->localCurrencyViewModel();
        beginResetModel();
        initAssets(viewModel->assetsModel());
        initBalance(viewModel->assetsBalance());
        _chainDataSource = viewModel->chainDataSource();
        _dexStatusManager = viewModel->dexStatusManager();
        connect(_dexStatusManager, &DexStatusManager::dexStatusChanged, this,
            &WalletAssetsListModel::onDexStatusChanged);
        endResetModel();

        accountBalanceChanged();
        countChanged();
    } else {
        Q_ASSERT_X(false, "WalletAssetsListModel::initialize",
            "No ApplicationViewModel, something is wrong");
    }
}

//==============================================================================

void WalletAssetsListModel::initAssets(WalletAssetsModel* assetModel)
{
    _assetModel = assetModel;
    _impl->_assets = assetModel->assets();
    for (auto&& asset : _assetModel->assets()) {
        auto assetID = asset.coinID();
        auto settings = _assetModel->assetSettings(assetID);
        connect(settings, &AssetSettings::activeChanged, this,
            [this, assetID] { onAssetIsActiveChanged(assetID); });
    }
}

//==============================================================================

void WalletAssetsListModel::initBalance(AssetsBalance* assetsBalance)
{

    _balance = assetsBalance;
    _impl->updateCache(_balance);
    connect(assetsBalance, &AssetsBalance::balanceChanged, this,
        &WalletAssetsListModel::onAssetBalanceUpdated);
    connect(assetsBalance, &AssetsBalance::localBalanceChanged, this,
        &WalletAssetsListModel::onAssetLocalBalanceChanged);
    connect(assetsBalance, &AssetsBalance::confirmedBalanceChanged, this,
        &WalletAssetsListModel::onAssetConfirmedBalanceUpdated);
}

//==============================================================================

Balance WalletAssetsListModel::getAssetTotalBalance(AssetID assetID) const
{
    return _impl->_balance.count(assetID) > 0 ? _impl->_balance.at(assetID).total() : 0;
}

//==============================================================================

Balance WalletAssetsListModel::getAssetBalanceOnChain(AssetID assetID) const
{
    return _impl->_balance.count(assetID) > 0 ? _impl->_balance.at(assetID).balance : 0;
}

//==============================================================================

Balance WalletAssetsListModel::getAssetNodeBalance(AssetID assetID) const
{
    return _impl->_balance.count(assetID) > 0 ? _impl->_balance.at(assetID).nodeBalance : 0;
}

//==============================================================================

Balance WalletAssetsListModel::getAssetAvailableNodeBalance(AssetID assetID) const
{
    return _impl->_balance.count(assetID) > 0 ? _impl->_balance.at(assetID).availableNodeBalance
                                              : 0;
}

//==============================================================================

unsigned WalletAssetsListModel::getSatsPerByte(AssetID assetID) const
{
    return _assetModel->assetSettings(assetID)->satsPerByte();
}

//==============================================================================

Enums::DexTradingStatus WalletAssetsListModel::getDexStatus(AssetID assetID) const
{
    if (_dexStatusManager) {
        return _dexStatusManager->dexStatusById(assetID);
    }
    return Enums::DexTradingStatus::Offline;
}

//==============================================================================

Balance WalletAssetsListModel::getActiveNodeBalance(AssetID assetID) const
{
    return _impl->_balance.count(assetID) > 0 ? _impl->_balance.at(assetID).activeNodeBalance : 0;
}

//==============================================================================

Balance WalletAssetsListModel::getConfirmedAssetBalanceOnChain(AssetID assetID) const
{
    return _impl->_balance.count(assetID) > 0 ? _impl->_balance.at(assetID).confirmedBalance : 0;
}

//==============================================================================

int WalletAssetsListModel::getPortfolioPercentage(Balance balance) const
{
    return _impl->_balanceSum != 0 ? static_cast<int>(balance * 100 / _impl->_balanceSum) : 0;
}

//==============================================================================

double WalletAssetsListModel::getPortfolioLocalPercentage(Balance balance, AssetID assetID) const
{

    if (assetID == 384) {
        double result = _impl->_balanceLocalSum > 0 ? _localCurrencyModel->convertSats(assetID, balance)
                / static_cast<double>(COIN) * 100.0 / _impl->_balanceLocalSum
                                                    : 0;
    }

    return _impl->_balanceLocalSum > 0 ? _localCurrencyModel->convertSats(assetID, balance)
            / static_cast<double>(COIN) * 100.0 / _impl->_balanceLocalSum
                                       : 0;
}

//==============================================================================

QString WalletAssetsListModel::isAssetActive(AssetID assetID) const
{
    return _assetModel->assetSettings(assetID)->isActive() ? "1" : "0";
}

//==============================================================================

void WalletAssetsListModel::sortByColumn(QString columnName)
{
    beginResetModel();

    if (columnName == "Balance") {
        std::multimap<Balance, CoinAsset> assetsVector;
        for (auto coinAsset : _impl->_assets) {
            assetsVector.insert({ _impl->_balance.at(coinAsset.coinID()).total(), coinAsset });
        }

        std::vector<CoinAsset> sortedAssets;
        for (auto asset : assetsVector) {
            sortedAssets.push_back(asset.second);
        }

        _impl->_assets.swap(sortedAssets);
    } else if (columnName == "Currency") {
        std::sort(std::begin(_impl->_assets), std::end(_impl->_assets),
            [](const CoinAsset& left, const CoinAsset& right) {
                return left.name() < right.name();
            });
    }

    endResetModel();
}

//==============================================================================

void WalletAssetsListModel::onAssetBalanceUpdated(AssetID assetID)
{
    auto it = std::find_if(std::begin(_impl->_assets), std::end(_impl->_assets),
        [assetID](const auto& asset) { return assetID == asset.coinID(); });

    if (it == std::end(_impl->_assets)) {
        return;
    }

    QModelIndex modelIndex = index(std::distance(std::begin(_impl->_assets), it));
    _impl->updateCache(_balance);
    dataChanged(modelIndex, modelIndex,
        { BalanceRole, NodeBalanceRole, BalanceOnChainRole, ActiveNodeBalanceRole,
            ConfirmedBalanceOnChainRole });
    accountBalanceChanged();
}

//==============================================================================

void WalletAssetsListModel::onAssetLocalBalanceChanged(AssetID assetID)
{
    auto it = std::find_if(std::begin(_impl->_assets), std::end(_impl->_assets),
        [assetID](const auto& asset) { return assetID == asset.coinID(); });

    if (it == std::end(_impl->_assets)) {
        return;
    }

    _impl->_balanceLocalSum = _balance->balanceLocalSum();
    dataChanged(index(0), index(_impl->_assets.size() - 1), { PortfolioPercentageRole });
}

//==============================================================================

void WalletAssetsListModel::onDexStatusChanged(AssetID assetID)
{
    auto it = std::find_if(_impl->_assets.begin(), _impl->_assets.end(),
        [assetID](const auto& asset) { return assetID == asset.coinID(); });

    if (it != std::end(_impl->_assets)) {
        auto modelIndex = index(std::distance(std::begin(_impl->_assets), it));
        dataChanged(modelIndex, modelIndex, { DexStatusRole });
    }
}

//==============================================================================

void WalletAssetsListModel::onAssetConfirmedBalanceUpdated(AssetID assetID)
{
    auto it = std::find_if(std::begin(_impl->_assets), std::end(_impl->_assets),
        [assetID](const auto& asset) { return assetID == asset.coinID(); });

    if (it == std::end(_impl->_assets)) {
        return;
    }

    QModelIndex modelIndex = index(std::distance(std::begin(_impl->_assets), it));
    _impl->updateCache(_balance);
    dataChanged(modelIndex, modelIndex,
        { ConfirmedBalanceOnChainRole, BalanceRole, PortfolioPercentageRole });
}

//==============================================================================

void WalletAssetsListModel::onLightningBalanceUpdated()
{
    // TODO: change hardcode for XSN only

    const auto& assets = _impl->_assets;
    for (size_t i = 0; i < assets.size(); ++i) {
        if (assets.at(i).coinID() == 0 || assets.at(i).coinID() == 2) {
            emit dataChanged(index(i), index(i));
        }
    }
}

//==============================================================================
