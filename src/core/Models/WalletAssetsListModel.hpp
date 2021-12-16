#ifndef WALLETASSETSLISTMODEL_HPP
#define WALLETASSETSLISTMODEL_HPP

#include <QAbstractListModel>
#include <QHash>
#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>
#include <memory>
#include <vector>

class WalletAssetsModel;
class AssetsBalance;
class LocalCurrencyViewModel;
class AbstractChainDataSource;
struct CoinAsset;
class DexStatusManager;

class WalletAssetsListModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
    Q_PROPERTY(int activeAssetsCount READ activeAssetsCount NOTIFY activeAssetsCountChanged)

public:
    enum Roles {
        IDRole,
        NameRole,
        TicketRole,
        ColorRole,
        BalanceRole,
        PortfolioPercentageRole,
        DescriptionRole,
        IsActiveRole,
        OfficialLinkRole,
        RedditLinkRole,
        TwitterLinkRole,
        TelegramLinkRole,
        IsAlwaysActiveRole,
        SymbolNameFilterRole,
        ConfirmationsForApproved,
        LocalBalanceRole,
        BalanceOnChainRole,
        NodeBalanceRole,
        MinLndCapacityRole,
        MaxLndCapacityRole,
        AverageSycBlockForSecRole,
        MinPaymentAmountRole,
        MaxPaymentAmountRole,
        SatsPerByteRole,
        ConfirmationsForChannelApproved,
        AvailableNodeBalanceRole,
        RentalChannelsPerCoinRole,
        DexStatusRole,
        ActiveNodeBalanceRole,
        AverageTxFeeRole,
        ConfirmedBalanceOnChainRole,
        ChainIDRole
    };

    explicit WalletAssetsListModel(QObject* parent = nullptr);
    virtual ~WalletAssetsListModel() override;

    virtual int rowCount(const QModelIndex& parent) const override final;
    virtual QVariant data(const QModelIndex& index, int role) const override final;
    virtual QHash<int, QByteArray> roleNames() const override final;

    int count() const;
    int activeAssetsCount() const;

signals:
    void accountBalanceChanged();
    void countChanged();
    void lightningBalanceChanged();
    void activeAssetsCountChanged();
    void averageFeeForAssetFinished(AssetID assetID, double value);

public slots:
    void initialize(QObject* appViewModel);
    void sortByColumn(QString columnName);
    void onLightningBalanceUpdated();
    QVariantMap get(int row);
    long getInitial(AssetID assetID);
    void toogleAssetActiveState(AssetID assetID);
    void changeAssetSatsPerByte(AssetID assetID, unsigned newValue);
    void averageFeeForAsset(AssetID assetID);

private slots:
    void onAssetIsActiveChanged(AssetID assetID);
    void onAssetBalanceUpdated(AssetID assetID);
    void onAssetLocalBalanceChanged(AssetID assetID);
    void onDexStatusChanged(AssetID assetID);
    void onAssetConfirmedBalanceUpdated(AssetID assetID);

private:
    void initAssets(WalletAssetsModel* assetModel);
    void initBalance(AssetsBalance* assetsBalance);
    void initLocalCurrencyViewModel(QObject* localCurrencyViewModel);
    Balance getAssetTotalBalance(AssetID assetID) const;
    int getPortfolioPercentage(Balance balance) const;
    double getPortfolioLocalPercentage(Balance balance, AssetID assetID) const;
    QString isAssetActive(AssetID assetID) const;
    int addressType(AssetID assetID) const;
    Balance getAssetBalanceOnChain(AssetID assetID) const;
    Balance getAssetNodeBalance(AssetID assetID) const;
    Balance getAssetAvailableNodeBalance(AssetID assetID) const;
    unsigned getSatsPerByte(AssetID assetID) const;
    Enums::DexTradingStatus getDexStatus(AssetID assetID) const;
    Balance getActiveNodeBalance(AssetID assetID) const;
    Balance getConfirmedAssetBalanceOnChain(AssetID assetID) const;

private:
    struct WalletAssetsListModelImpl;
    std::unique_ptr<WalletAssetsListModelImpl> _impl;
    QPointer<AssetsBalance> _balance;
    QPointer<WalletAssetsModel> _assetModel;
    QPointer<LocalCurrencyViewModel> _localCurrencyModel;
    QPointer<AbstractChainDataSource> _chainDataSource;
    QPointer<DexStatusManager> _dexStatusManager;
};

#endif // WALLETASSETSLISTMODEL_HPP
