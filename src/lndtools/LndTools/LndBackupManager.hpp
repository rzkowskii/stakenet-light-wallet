#ifndef LNDBACKUPMANAGER_HPP
#define LNDBACKUPMANAGER_HPP

#include <QDateTime>
#include <QDir>
#include <QObject>
#include <QPointer>
#include <boost/optional.hpp>

class QTimer;
class LndGrpcClient;

class LndBackupManager : public QObject {
    Q_OBJECT
public:
    explicit LndBackupManager(LndGrpcClient* client, unsigned assetID, QString defaultBackupDir,
        QObject* parent = nullptr);
    ~LndBackupManager() override;

    struct Backup {
        QStringList chanPoints;
        QDateTime timestamp;
        QByteArray bytes;
        int assetID{ -1 };

        static Backup ReadFromBlob(QByteArray blob);
        QByteArray writeToBlob() const;
    };

    bool hasLatestBackup() const;
    const Backup& latestBackup() const;

    static boost::optional<Backup> ReadBackupFromFile(QString backupPath);

    bool exportBackupFile(QString to);

    void setBackupDir(QString backupDirPath);
    QDir backupDir() const;
    QDir defaultBackupDir() const;

signals:
    void latestBackupChanged();
    void backupDirChanged();

private slots:
    void onConnected();

private:
    void init();
    void setLatestBackup(Backup newBackup);
    void setBackupFromDir(QString backupDir);
    QString backupOutFile() const;

private:
    QPointer<LndGrpcClient> _client;
    QDir _defaultBackupDir;
    QString _backupDirPath;
    Backup _latestBackup;
    QTimer* _connectionTimer{ nullptr };
    unsigned _assetID;
    bool _hasConnection{ false };
};

#endif // LNDBACKUPMANAGER_HPP
