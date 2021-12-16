#ifndef ASSETUTXOBALANCEPROVIDER_HPP
#define ASSETUTXOBALANCEPROVIDER_HPP

#include <Data/AbstractAssetBalanceProvider.hpp>
#include <Data/CoinAsset.hpp>
#include <Data/TransactionEntry.hpp>
#include <QObject>
#include <QPointer>

class ChainView;
class AssetsTransactionsCache;
class AssetTransactionsDataSource;
class AbstractChainManager;
class LnDaemonInterface;

class AssetUTXOBalanceProvider : public AbstractAssetBalanceProvider {
    Q_OBJECT
public:
    explicit AssetUTXOBalanceProvider(CoinAsset coinAsset,
        QPointer<AssetsTransactionsCache> assetsTxnsCache,
        QPointer<AbstractChainManager> chainManager, QPointer<LnDaemonInterface> lndInterface,
        QObject* parent = nullptr);

    void update() override;

private slots:
    void onTxnsAdded(const std::vector<Transaction>& txns);
    void onTxnsChanged(const std::vector<Transaction>& txns);
    void onChainHeightChanged();

private:
    void init(AssetsTransactionsCache& transactionsCache);
    void updateBalance();
    Balance getConfirmedBalance(const OnChainTxRef& transaction) const;

private:
    CoinAsset _asset;
    QPointer<AssetTransactionsDataSource> _dataSource;
    QPointer<AbstractChainManager> _chainManager;
    QPointer<LnDaemonInterface> _lndInterface;
    std::shared_ptr<ChainView> _chainView;
    std::optional<int64_t> _chainHeight;
};

#endif // ASSETUTXOBALANCEPROVIDER_HPP
