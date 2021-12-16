#ifndef LIGHTNINGCHANNELBACKUPVIEWMODEL_HPP
#define LIGHTNINGCHANNELBACKUPVIEWMODEL_HPP

#include <QObject>
#include <QVariantMap>
#include <Tools/Common.hpp>

class LndBackupManager;
class ApplicationViewModel;
class LnDaemonInterface;
class AssetSettings;

class LightningChannelBackupViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(QVariantMap backupMeta READ backupMeta NOTIFY backupMetaChanged)
    Q_PROPERTY(QString backupPath READ backupPath NOTIFY backupPathChanged)

public:
    explicit LightningChannelBackupViewModel(QObject* parent = nullptr);

    QVariantMap backupMeta() const;
    QString backupPath() const;

signals:
    void backupMetaChanged();
    void backupPathChanged();
    void lnChannelsBackupRestored();
    void lnChannelsBackupRestoreFailed();
    void lnChannelsBackupExported();
    void lnChannelsBackupExportFailed();

public slots:
    void initialize(ApplicationViewModel* appViewModel, AssetID assetID);
    void initializeFromFile(
        ApplicationViewModel* appViewModel, AssetID assetID, QString backupPath);
    void restoreFromBackup();
    void exportBackup(QString to);
    void changeBackupDir(QString toPath);

private:
private:
    QPointer<LnDaemonInterface> _lndManager;
    QPointer<LndBackupManager> _lndBackupManager;
    QPointer<AssetSettings> _assetSettings;
    QVariantMap _backupMeta;
    QString _backupPath;
    std::string _bytes;
    size_t _uniqueChannelsCount{ 0 };
};

#endif // LIGHTNINGCHANNELBACKUPVIEWMODEL_HPP
