#ifndef walletViewModel_HPP
#define walletViewModel_HPP

#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>
#include <support/allocators/secure.h>

class WalletDataSource;
class ApplicationViewModel;
class AssetsTransactionsCache;
class AssetsAccountsCache;
class SyncService;
class WalletAssetsModel;
class AbstractMutableChainManager;
class UTXOSetDataSource;

class WalletViewModel : public QObject {
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
public:
    explicit WalletViewModel(WalletDataSource& dataSource, UTXOSetDataSource& utxoDataSource,
        AssetsTransactionsCache& transactionsCache, AssetsAccountsCache& accountsCache, WalletAssetsModel& walletAssetsModel,
        AbstractMutableChainManager& chainManager, SyncService& syncService,
        QObject* parent = nullptr);
    ~WalletViewModel();

    enum class LoadingState { Chain, Transactions, Wallet, Done };
    Q_ENUM(LoadingState)

public slots:
    void createWalletWithMnemonic();
    void requestMnemonic();

    bool verifyMnemonic(QString mnemonic);
    void restoreWallet(QString mnemonic);
    void encryptWallet(QString password);
    void loadApp(QString password = QString());
    QString walletAge();

    void resetState(LoadingState loadingState);

signals:
    void requestMnemonicFinished(QString mnemonic);

    void loadedChanged(bool isLoaded);
    void createdChanged(bool isCreated);
    void restoredChanged(bool isRestored);

    void walletEncrypted();
    void encryptWalletFailed(QString error);

    void loadingProcessChanged(LoadingState state);
    void loadingProcessFailed(LoadingState state);
    void transactionsDatabaseLoaded();
    void walletLoaded();
    void loadingWalletFailed(QString error);

private slots:
    void startSync();

private:
    Promise<void> loadChains();
    Promise<void> loadTransactionsDatabase();
    Promise<void> loadAccountsDatabase();
    Promise<void> loadWallet(SecureString password);
    Promise<std::vector<AssetID>> filteredChains(std::vector<AssetID> chains);
    std::tuple<bool, QString> canBootstrap() const;

private:
    WalletDataSource& _walletDataSource;
    UTXOSetDataSource& _utxoDataSource;
    AssetsTransactionsCache& _transactionsCache;
    AssetsAccountsCache& _accountsCache;
    WalletAssetsModel& _walletAssetsModel;
    AbstractMutableChainManager& _chainManager;
    SyncService& _syncService;
};
#endif // walletViewModel_HPP
