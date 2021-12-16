#ifndef WALLETTRANSACTIONSLISTMODEL_HPP
#define WALLETTRANSACTIONSLISTMODEL_HPP

#include <QAbstractListModel>
#include <QObject>
#include <QPointer>

#include <Models/WalletDataSource.hpp>

class AssetTransactionsDataSource;
class WalletAssetsModel;
class AbstractChainManager;
class ChainView;

class WalletTransactionsListModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit WalletTransactionsListModel(QPointer<AssetTransactionsDataSource> dataSource,
        QPointer<WalletAssetsModel> walletAssetsModel, QPointer<AbstractChainManager> chainManager,
        QObject* parent = nullptr);
    ~WalletTransactionsListModel() override;

    enum class TransactionType { OnChain, LightningInvoice, LightningPayment, EthOnChainTx , Undefined };
    Q_ENUM(TransactionType)

    enum Roles {
        TransactionTypeRole,
        TransactionIDRole,
        DeltaRole,
        TxDateRole,
        CurrencyRole,
        SymbolRole,
        TxAmountRole,
        ActivityRole,
        StatusRole,
        TxDataRole,
        IsDexTxRole
    };
    Q_ENUMS(Roles)

    virtual int rowCount(const QModelIndex& parent) const override final;
    virtual QVariant data(const QModelIndex& index, int role) const override final;
    virtual QHash<int, QByteArray> roleNames() const override final;

public slots:

private slots:
    void onTransactionsFetched();
    void onTxnsAdded(const std::vector<Transaction>& txns);
    void onTxnsChanged(const std::vector<Transaction>& txns, std::vector<int> indexes);
    void updateConfirmations();
    void onChainsLoaded();

private:
    void init();
    QVariantList getAddresses(const OnChainTx& transaction) const;
    int getConfirmations(const OnChainTx& transaction) const;
    QString getOnChainTxActivity(const OnChainTx& transaction) const;
    int getConfirmations(const EthOnChainTx& transaction) const;
    QString getOnChainTxActivity(const EthOnChainTx& transaction) const;
    QVariantList getAddresses(const EthOnChainTx& transaction) const;


private:
    QPointer<AssetTransactionsDataSource> _dataSource;
    QPointer<WalletAssetsModel> _walletAssetsModel;
    QPointer<AbstractChainManager> _chainManager;
    std::shared_ptr<ChainView> _chainView;
    std::map<BlockHash, size_t> _blockConfirmations;
    size_t _tipHeight{ 0 };
    size_t _rowCount{ 0 };
};

#endif // WALLETTRANSACTIONSLISTMODEL_HPP
