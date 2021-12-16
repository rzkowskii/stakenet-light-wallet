#ifndef WALLETASSETVIEWMODEL_HPP
#define WALLETASSETVIEWMODEL_HPP

#include <Models/WalletDataSource.hpp>
#include <Tools/Common.hpp>

#include <QObject>
#include <QPointer>
#include <boost/optional.hpp>
#include <memory>

class QAbstractListModel;
class WalletTransactionsListModel;
class AssetsTransactionsCache;
class WalletAssetsModel;
class ApplicationViewModel;
class AbstractChainManager;
class ReceiveTransactionViewModel;
class WalletDataSource;
class AccountDataSource;
class AssetsBalance;

class WalletAssetViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(unsigned currentAssetID READ currentAssetID WRITE setCurrentAssetID NOTIFY
            currentAssetIDChanged)
    Q_PROPERTY(QObject* transactionsListModel READ transactionsListModel NOTIFY
            transactionsListModelChanged)
    Q_PROPERTY(QObject* receiveTxViewModel READ receiveTxViewModel NOTIFY receiveTxViewModelChanged)
    Q_PROPERTY(QVariantMap assetBalance READ assetBalance NOTIFY assetBalanceChanged)
    Q_PROPERTY(QVariantMap assetInfo READ assetInfo NOTIFY assetInfoChanged)

public:
    explicit WalletAssetViewModel(QObject* parent = nullptr);
    ~WalletAssetViewModel();

    QObject* transactionsListModel();
    QObject* receiveTxViewModel();
    AssetID currentAssetID() const;
    QVariantMap assetBalance() const;
    QVariantMap assetInfo() const;

    void setCurrentAssetID(AssetID assetID);

signals:
    void currentAssetIDChanged();
    void transactionsListModelChanged();
    void receiveTxViewModelChanged();
    void assetBalanceChanged();
    void assetInfoChanged();

public slots:
    void initialize(ApplicationViewModel* applicationViewModel);
    QString buildUrlForExplorer(QString txid);

private slots:
    void onCurrentAssetIDChanged();

private:
    void initAssetsBalance(AssetsBalance* assetsBalance);
    void updateAssetBalance();
    void updateAssetInfo();

private:
    boost::optional<AssetID> _currentAssetID;
    QPointer<AssetsTransactionsCache> _dataSource;
    QPointer<AssetsBalance> _balance;
    QPointer<WalletAssetsModel> _walletAssetsModel;
    QPointer<WalletDataSource> _walletDataSource;
    AccountDataSource* _accountDataSource;
    std::unique_ptr<ReceiveTransactionViewModel> _receiveTransactionViewModel;
    using TransactionsListModelPtr = std::unique_ptr<WalletTransactionsListModel>;
    std::map<AssetID, TransactionsListModelPtr> _walletTransactionsListModels;
    QPointer<AbstractChainManager> _chainManager;
    QVariantMap _assetBalance;
    QVariantMap _assetInfo;
};

#endif // WALLETASSETVIEWMODEL_HPP
