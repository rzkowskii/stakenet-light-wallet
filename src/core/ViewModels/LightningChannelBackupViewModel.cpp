#include "LightningChannelBackupViewModel.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <LndTools/LndBackupManager.hpp>
#include <Models/LnDaemonInterface.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <QUrl>
#include <ViewModels/ApplicationViewModel.hpp>

//==============================================================================

LightningChannelBackupViewModel::LightningChannelBackupViewModel(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

QVariantMap LightningChannelBackupViewModel::backupMeta() const
{
    return _backupMeta;
}

//==============================================================================

void LightningChannelBackupViewModel::initialize(
    ApplicationViewModel* appViewModel, AssetID assetID)
{
    _lndBackupManager = appViewModel->paymentNodesManager()->lnDaemonManager()->backupManagerById(assetID);
    _assetSettings = appViewModel->assetsModel()->assetSettings(assetID);

    if (_lndBackupManager) {
        auto onLatestBackupChanged = [this] {
            QVariantMap backupMeta;
            backupMeta["assetID"] = _lndBackupManager->latestBackup().assetID;
            backupMeta["timestamp"]
                = _lndBackupManager->latestBackup().timestamp.toString("yyyy-MM-dd HH:mm:ss");
            backupMeta["outpoints"] = _lndBackupManager->latestBackup().chanPoints;

            _bytes = _lndBackupManager->latestBackup().bytes.toStdString();

            _backupMeta.swap(backupMeta);
            backupMetaChanged();
        };

        auto onBackupDirChanged = [this] {
            _backupPath = _lndBackupManager->backupDir().path();
            backupPathChanged();
        };

        connect(
            _lndBackupManager, &LndBackupManager::latestBackupChanged, this, onLatestBackupChanged);
        if (_lndBackupManager->hasLatestBackup()) {
            onLatestBackupChanged();
        }
        onBackupDirChanged();

        connect(_lndBackupManager, &LndBackupManager::backupDirChanged, this, onBackupDirChanged);
    }

    _lndManager = appViewModel->paymentNodesManager()->lnDaemonManager()->interfaceById(assetID);
}

//==============================================================================

void LightningChannelBackupViewModel::initializeFromFile(
    ApplicationViewModel* appViewModel, AssetID assetID, QString backupPath)
{
    _lndBackupManager = appViewModel->paymentNodesManager()->lnDaemonManager()->backupManagerById(assetID);

    QString localFilePath = QUrl(backupPath).toLocalFile();
    bool isLocalFile = localFilePath.isEmpty();

    QString filePath = isLocalFile ? backupPath : QUrl(backupPath).toLocalFile();

    if (_lndBackupManager) {
        QVariantMap backupMeta;
        auto backup = _lndBackupManager->ReadBackupFromFile(filePath).get();

        backupMeta["assetID"] = backup.assetID;
        backupMeta["timestamp"] = backup.timestamp.toString("yyyy-MM-dd HH:mm:ss");
        backupMeta["outpoints"] = backup.chanPoints;

        _bytes = backup.bytes.toStdString();
        _backupMeta.swap(backupMeta);
        backupMetaChanged();
    }

    _lndManager = appViewModel->paymentNodesManager()->lnDaemonManager()->interfaceById(assetID);
}

//==============================================================================

void LightningChannelBackupViewModel::restoreFromBackup()
{
    _lndManager->restoreChannelBackups(_bytes)
        .then([this] { lnChannelsBackupRestored(); })
        .fail([this] { lnChannelsBackupRestoreFailed(); });
}

//==============================================================================

void LightningChannelBackupViewModel::exportBackup(QString to)
{
    QString localFilePath = QUrl(to).toLocalFile();
    bool isLocalFile = localFilePath.isEmpty();

    QString filePath = isLocalFile ? to : QUrl(to).toLocalFile();

    if (_lndBackupManager->exportBackupFile(filePath)) {
        lnChannelsBackupExported();
    } else {
        lnChannelsBackupExportFailed();
    }
}

//==============================================================================

void LightningChannelBackupViewModel::changeBackupDir(QString toPath)
{
    QString localFilePath = QUrl(toPath).toLocalFile();
    bool isLocalFile = localFilePath.isEmpty();

    QString filePath = isLocalFile ? toPath : QUrl(toPath).toLocalFile();

    _assetSettings->setBackupDir(filePath);
}

//==============================================================================

QString LightningChannelBackupViewModel::backupPath() const
{
    return _backupPath;
}

//==============================================================================
