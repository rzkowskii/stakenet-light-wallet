#include "LnDaemonsManager.hpp"
#include <Chain/AbstractTransactionsCache.hpp>
#include <Data/LnPaymentsProxy.hpp>
#include <LndTools/LndBackupManager.hpp>
#include <LndTools/LndGrpcClient.hpp>
#include <Models/AutopilotModel.hpp>
#include <Models/LnDaemonStateModel.hpp>
#include <Tools/AppConfig.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

LnDaemonsManager::LnDaemonsManager(Utils::WorkerThread& workerThread,
    AssetsTransactionsCache& txCache, const WalletAssetsModel& assetsModel, QObject* parent)
    : QObject(parent)
    , _workerThread(workerThread)
    , _assetsModel(assetsModel)
{
    init(txCache);
}

//==============================================================================

LnDaemonsManager::~LnDaemonsManager()
{
}

//==============================================================================

LnDaemonInterface* LnDaemonsManager::interfaceById(AssetID assetID) const
{
    return _daemonsInterfaces.count(assetID) > 0 ? _daemonsInterfaces.at(assetID).get() : nullptr;
}

//==============================================================================

AutopilotModel* LnDaemonsManager::autopilotById(AssetID assetID) const
{
    return _autopilotModels.count(assetID) > 0 ? _autopilotModels.at(assetID).get() : nullptr;
}

//==============================================================================

LndBackupManager* LnDaemonsManager::backupManagerById(AssetID assetID) const
{
    return _backupManagers.count(assetID) > 0 ? _backupManagers.at(assetID).get() : nullptr;
}

//==============================================================================

LnPaymentsProxy* LnDaemonsManager::paymentsInterfaceById(AssetID assetID) const
{
    return _paymentSyncManagers.count(assetID) > 0 ? _paymentSyncManagers.at(assetID).get()
                                                   : nullptr;
}

//==============================================================================

void LnDaemonsManager::init(AssetsTransactionsCache& txCache)
{
    // TODO(yuraolex): in future we need to get this value from active assets I would say
    auto supportedLnds = std::vector<AssetID>{ 0, 2, 384 };
    const auto& appConfig = AppConfig::Instance().config();

    auto dataDir = GetDataDir(ApplicationViewModel::IsEmulated());

    for (auto&& assetID : supportedLnds) {
        auto daemonConfig = appConfig.lndConfig.daemonConfigById(assetID);

        QDir rootLndDir(dataDir.absoluteFilePath(QString("lnd/%1").arg(appConfig.dataDirPath)));
        QDir chainLndDir(rootLndDir.absoluteFilePath(daemonConfig.assetSymbol));
        chainLndDir.mkpath(".");

        if (daemonConfig.tlsCert.isEmpty()) {
            daemonConfig.tlsCert = chainLndDir.absoluteFilePath("tls.cert");
        }

        if (daemonConfig.tlsKey.isEmpty()) {
            daemonConfig.tlsKey = chainLndDir.absoluteFilePath("tls.key");
        }

        if (daemonConfig.macaroonPath.isEmpty()) {
            daemonConfig.macaroonPath = chainLndDir.absoluteFilePath("admin.macaroon");
        }

        LnDaemonInterface::Cfg config(daemonConfig, chainLndDir.absolutePath(), appConfig.rpcPort);

        std::unique_ptr<LndGrpcClient> client(new LndGrpcClient(config.daemonConfig.rpcListenHost,
            [certPath = daemonConfig.tlsCert] { return Utils::ReadCert(certPath); },
            [macaroonPath = daemonConfig.macaroonPath] {
                return Utils::ReadMacaroon(macaroonPath).toStdString();
            }));

        qobject_delete_later_unique_ptr<LnDaemonInterface> interface(new LnDaemonInterface(
            config, _assetsModel.assetById(assetID).lndData(), std::move(client)));
        qobject_delete_later_unique_ptr<AutopilotModel> autopilot(
            new AutopilotModel(assetID, _assetsModel, interface->grpcClient()));

        QString backupDir = chainLndDir.absolutePath();

        // this will initiate backuping process
        qobject_delete_later_unique_ptr<LndBackupManager> backupManager(
            new LndBackupManager(interface->grpcClient(), assetID, backupDir));

        auto assetSettings = _assetsModel.assetSettings(assetID);
        connect(assetSettings, &AssetSettings::backupDirChanged, backupManager.get(),
            &LndBackupManager::setBackupDir);
        backupManager->setBackupDir(assetSettings->backupDir());

        txCache.cacheById(assetID).then(
            [this, assetID, client = interface->grpcClient()](AbstractTransactionsCache* cache) {
                qobject_delete_later_unique_ptr<LnPaymentsProxy> paymentsSyncManager(
                    new LnPaymentsProxy(assetID, cache, client));
                paymentsSyncManager->moveToThread(&_workerThread);
                _paymentSyncManagers.emplace(assetID, std::move(paymentsSyncManager));
            });

        _autopilotModels.emplace(assetID, std::move(autopilot));
        _daemonsInterfaces.emplace(assetID, std::move(interface));
        _backupManagers.emplace(assetID, std::move(backupManager));
    }
}

//==============================================================================
