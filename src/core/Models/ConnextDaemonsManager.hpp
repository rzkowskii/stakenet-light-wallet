#ifndef CONNEXTDAEMONSMANAGER_HPP
#define CONNEXTDAEMONSMANAGER_HPP

#include <QObject>
#include <unordered_map>

#include <LndTools/ConnextProcessManager.hpp>
#include <Models/ConnextDaemonInterface.hpp>
#include <Tools/AppConfig.hpp>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>

class WalletAssetsModel;
class ConnextDaemonInterface;
class AccountDataSource;
class ConnextBrowserNodeApi;
class SendAccountTransactionModel;
class AbstractNetworkingFactory;
class AssetsTransactionsCache;
class ConnextPaymentsProxy;

class ConnextDaemonsManager : public QObject {
    Q_OBJECT
public:
    explicit ConnextDaemonsManager(Utils::WorkerThread& workerThread,
        WalletAssetsModel& assetsModel, AccountDataSource& dataSource,
        AbstractNetworkingFactory& apiClientFactory, QPointer<AssetsTransactionsCache> txCache,
        QObject* parent = nullptr);
    ~ConnextDaemonsManager();

    ConnextBrowserNodeApi* browserNodeApi() const;
    ConnextDaemonInterface* interfaceById(AssetID assetID) const;
    ConnextProcessManager* processManager() const;

private:
    void init();
    void registerConnextConf(ConnextDaemonConfig config);
    void tryInitNode();
    void setNodeInitialized(bool value);

private:
    Utils::WorkerThread& _workerThread;
    WalletAssetsModel& _assetsModel;
    AccountDataSource& _accountDataSource;
    AbstractNetworkingFactory& _apiClientFactory;
    QPointer<AssetsTransactionsCache> _txCache;
    qobject_delete_later_unique_ptr<ConnextProcessManager> _processManager;
    std::unordered_map<AssetID, qobject_delete_later_unique_ptr<ConnextDaemonInterface>>
        _daemonsInterfaces;
    ConnextBrowserNodeApi* _browserNodeApi{ nullptr };
    QTimer* _connextInitTimer{ nullptr };
    bool _nodeInitialized{ false };
    std::unordered_map<AssetID, qobject_delete_later_unique_ptr<ConnextPaymentsProxy>>
        _paymentManagers;
};

#endif // CONNEXTDAEMONSMANAGER_HPP
