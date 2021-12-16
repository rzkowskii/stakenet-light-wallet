#include "WalletAssetViewModel.hpp"
#include <Chain/AbstractChainManager.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/AssetsBalance.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Models/AssetTransactionsDataSource.hpp>
#include <Models/WalletTransactionsListModel.hpp>
#include <ViewModels/ApplicationViewModel.hpp>
#include <ViewModels/ReceiveAccountTransactionViewModel.hpp>
#include <ViewModels/ReceiveTransactionViewModel.hpp>
#include <ViewModels/ReceiveUTXOTransactionViewModel.hpp>
#include <key_io.h>

//==============================================================================

WalletAssetViewModel::WalletAssetViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

WalletAssetViewModel::~WalletAssetViewModel() {}

//==============================================================================

QObject* WalletAssetViewModel::transactionsListModel()
{
    if (!_dataSource || !_currentAssetID)
        return nullptr;

    if (_walletTransactionsListModels.count(_currentAssetID.get()) == 0)
        return nullptr;

    return _walletTransactionsListModels.at(_currentAssetID.get()).get();
}

//==============================================================================

QObject* WalletAssetViewModel::receiveTxViewModel()
{
    return _receiveTransactionViewModel.get();
}

//==============================================================================

AssetID WalletAssetViewModel::currentAssetID() const
{
    return _currentAssetID.get_value_or(-1);
}

//==============================================================================

QVariantMap WalletAssetViewModel::assetBalance() const
{
    return _assetBalance;
}

//==============================================================================

QVariantMap WalletAssetViewModel::assetInfo() const
{
    return _assetInfo;
}

//==============================================================================

void WalletAssetViewModel::initialize(ApplicationViewModel* applicationViewModel)
{
    _dataSource = applicationViewModel->transactionsCache();
    _walletAssetsModel = applicationViewModel->assetsModel();
    _chainManager = applicationViewModel->chainManager();
    _walletDataSource = applicationViewModel->dataSource();
    _accountDataSource = applicationViewModel->accountDataSource();
    initAssetsBalance(applicationViewModel->assetsBalance());

    onCurrentAssetIDChanged();
}

//==============================================================================

QString WalletAssetViewModel::buildUrlForExplorer(QString txid)
{
    const auto& asset = _walletAssetsModel->assetById(currentAssetID());
    return asset.misc().explorerLink.arg(txid);
}

//==============================================================================

void WalletAssetViewModel::onCurrentAssetIDChanged()
{
    if (!_dataSource || !_currentAssetID) {
        return;
    }

    auto assetID = _currentAssetID.get();
    if (_walletTransactionsListModels.count(assetID) == 0) {
        _dataSource->cacheById(assetID).then([this, assetID](AbstractTransactionsCache* cache) {
            auto transactionsDataSource = new AssetTransactionsDataSource(assetID, cache, this);
            _walletTransactionsListModels.emplace(assetID,
                TransactionsListModelPtr(new WalletTransactionsListModel(
                    transactionsDataSource, _walletAssetsModel, _chainManager)));
            this->transactionsListModelChanged();
        });
    } else {
        this->transactionsListModelChanged();
    }

    const auto& asset = _walletAssetsModel->assetById(assetID);

    if (asset.type() == CoinAsset::Type::Invalid) {
        return;
    }

    if (asset.type() == CoinAsset::Type::UTXO) {
        _receiveTransactionViewModel
            = std::make_unique<ReceiveUTXOTransactionViewModel>(*_walletDataSource, assetID, false);
    } else {
        _receiveTransactionViewModel = std::make_unique<ReceiveAccountTransactionViewModel>(
            *_accountDataSource, assetID, false);
    }
    receiveTxViewModelChanged();

    updateAssetBalance();
    updateAssetInfo();
}

//==============================================================================

void WalletAssetViewModel::initAssetsBalance(AssetsBalance* assetsBalance)
{
    _balance = assetsBalance;
    connect(
        _balance, &AssetsBalance::balanceChanged, this, &WalletAssetViewModel::updateAssetBalance);
    connect(_balance, &AssetsBalance::localBalanceChanged, this,
        &WalletAssetViewModel::updateAssetBalance);
    connect(_balance, &AssetsBalance::confirmedBalanceChanged, this,
        &WalletAssetViewModel::updateAssetBalance);
}

//==============================================================================

void WalletAssetViewModel::updateAssetBalance()
{
    if (!_dataSource || !_currentAssetID)
        return;

    Balance total = _balance->balance().count(currentAssetID()) > 0
        ? _balance->balance().at(currentAssetID()).total()
        : 0;
    Balance onChain = _balance->balance().count(currentAssetID()) > 0
        ? _balance->balance().at(currentAssetID()).balance
        : 0;

    QVariantMap allBalances{
        { "total", static_cast<double>(total) },
        { "onChain", static_cast<double>(onChain) },
    };

    _assetBalance.swap(allBalances);
    assetBalanceChanged();
}

//==============================================================================

void WalletAssetViewModel::updateAssetInfo()
{

    if (!_dataSource || !_currentAssetID)
        return;

    auto assetID = _currentAssetID.get();
    const auto& asset = _walletAssetsModel->assetById(assetID);

    QVariantMap assetInfo{ { "name", asset.name() }, { "color", asset.misc().color },
        { "symbol", asset.ticket() },
        { "сonfirmationsForApproved", asset.misc().confirmationsForApproved },
        { "averageSycBlockForSec", asset.misc().averageSycBlockForSec },
        { "minLndCapacity", asset.lndData().minChannelCapacity },
        { "chainId", asset.token() ? asset.params().params.extCoinType() : asset.coinID() },
        { "isToken", asset.token().has_value() } };

    _assetInfo.swap(assetInfo);
    assetInfoChanged();
}

//==============================================================================

void WalletAssetViewModel::setCurrentAssetID(AssetID assetID)
{
    if (currentAssetID() != assetID) {
        if (assetID >= 0) {
            _currentAssetID = assetID;
            onCurrentAssetIDChanged();
            currentAssetIDChanged();
        } else {
            _currentAssetID.reset();
        }
    }
}

//==============================================================================
