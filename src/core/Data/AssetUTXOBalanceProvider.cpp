#include "AssetUTXOBalanceProvider.hpp"

#include <Chain/AbstractChainManager.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/Chain.hpp>
#include <Models/AssetTransactionsDataSource.hpp>
#include <Models/LnDaemonInterface.hpp>

static const int64_t MAX_CONFIRMATIONS = 99999999;

//==============================================================================

AssetUTXOBalanceProvider::AssetUTXOBalanceProvider(CoinAsset coinAsset,
    QPointer<AssetsTransactionsCache> assetsTxnsCache, QPointer<AbstractChainManager> chainManager,
    QPointer<LnDaemonInterface> lndInterface, QObject* parent)
    : AbstractAssetBalanceProvider(parent)
    , _asset(coinAsset)
    , _chainManager(chainManager)
    , _lndInterface(lndInterface)
{
    Q_ASSERT(assetsTxnsCache);
    init(*assetsTxnsCache);
}

//==============================================================================

void AssetUTXOBalanceProvider::update()
{
    if (_dataSource) {
        _dataSource->fetchTransactions();
    }
}

//==============================================================================

void AssetUTXOBalanceProvider::onTxnsAdded(const std::vector<Transaction>& txns)
{
    auto currentBalance = balance();
    for (const auto& var : txns) {
        if (auto tx = boost::variant2::get_if<OnChainTxRef>(&var)) {
            currentBalance.balance += (*tx)->delta();
            currentBalance.confirmedBalance += getConfirmedBalance(*tx);
        }
    }
    setBalance(currentBalance);
}

//==============================================================================

void AssetUTXOBalanceProvider::onTxnsChanged(const std::vector<Transaction>& txns)
{
    Q_UNUSED(txns);
    updateBalance();
}

//==============================================================================

void AssetUTXOBalanceProvider::onChainHeightChanged()
{
    if (_chainView) {
        QPointer<AssetUTXOBalanceProvider> self{ this };
        _chainView->chainHeight().then([self](size_t newHeight) {
            if (self) {
                self->_chainHeight = static_cast<int64_t>(newHeight);
                self->updateBalance();
            }
        });
    }
}

//==============================================================================

void AssetUTXOBalanceProvider::init(AssetsTransactionsCache& transactionsCache)
{
    auto assetID = _asset.coinID();
    transactionsCache.cacheById(assetID).then([this, assetID](AbstractTransactionsCache* cache) {
        _dataSource = new AssetTransactionsDataSource(assetID, cache, this);
        connect(_dataSource, &AssetTransactionsDataSource::transactionsFetched, this,
            &AssetUTXOBalanceProvider::updateBalance);
        connect(_dataSource, &AssetTransactionsDataSource::txnsAdded, this,
            &AssetUTXOBalanceProvider::onTxnsAdded);
        connect(_dataSource, &AssetTransactionsDataSource::txnsChanged, this,
            &AssetUTXOBalanceProvider::onTxnsChanged);
    });

    _chainManager
        ->getChainView(
            _asset.coinID(), AbstractChainManager::ChainViewUpdatePolicy::CompressedEvents)
        .then([this](std::shared_ptr<ChainView> chainView) {
            _chainView.swap(chainView);
            connect(_chainView.get(), &ChainView::bestBlockHashChanged, this,
                &AssetUTXOBalanceProvider::onChainHeightChanged);
            this->onChainHeightChanged();
        });

    connect(_lndInterface, &LnDaemonInterface::balanceChanged, this,
        [this](LnDaemonInterface::LnBalance newBalance) {
            AssetBalance result;
            result.nodeBalance = newBalance.total();
            result.availableNodeBalance = newBalance.availableTotal();
            result.activeNodeBalance = newBalance.active;
            result.balance = balance().balance;
            result.confirmedBalance = balance().confirmedBalance;
            this->setBalance(result);
        });
}

//==============================================================================

void AssetUTXOBalanceProvider::updateBalance()
{
    AssetBalance result;
    for (const auto& var : _dataSource->transactionsList()) {

        // TODO(yuraolex): optimize this later by providing new data source
        if (auto tx = boost::variant2::get_if<OnChainTxRef>(&var)) {
            result.balance += (*tx)->delta();
            result.confirmedBalance += getConfirmedBalance(*tx);
        }
    }
    result.nodeBalance = balance().nodeBalance;
    result.availableNodeBalance = balance().availableNodeBalance;
    result.activeNodeBalance = balance().activeNodeBalance;

    setBalance(result);
}

//==============================================================================

Balance AssetUTXOBalanceProvider::getConfirmedBalance(const OnChainTxRef& transaction) const
{
    if (!_chainHeight) {
        return Balance{ 0 };
    }

    int64_t minConfirmations = _asset.misc().confirmationsForApproved;
    auto height = *_chainHeight;

    Balance result{ 0 };
    if (height >= transaction->blockHeight()) {
        auto confirmations = height - transaction->blockHeight() + 1;
        if (transaction->blockHeight() < 0) {
            auto outputSum = std::accumulate(transaction->outputs().begin(),
                transaction->outputs().end(), Balance{ 0 },
                [](Balance accum, const auto& output) { return accum + output.value(); });
            result = transaction->delta() - outputSum;
        } else {
            if (confirmations >= minConfirmations && confirmations <= MAX_CONFIRMATIONS) {
                result = transaction->delta();
            }
        }
    }
    return result;
}

//==============================================================================
