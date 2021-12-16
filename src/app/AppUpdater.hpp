#ifndef APPUPDATER_HPP
#define APPUPDATER_HPP

#include <QJsonObject>
#include <QObject>

#include <QThread>
#include <QThreadPool>
#include <Tools/Common.hpp>
#include <UpdateConfig.hpp>
#include <Utils/Utils.hpp>

class AppUpdater : public QObject {
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(QString readyVersionStr READ readyVersionStr NOTIFY updaterStateChanged)
    Q_PROPERTY(UpdaterState updaterState READ updaterState NOTIFY updaterStateChanged)
public:
    enum class UpdaterState { Checking, Downloading, Ready, NotReady, Invalid, ExistNew, CheckingFailed, DownloadingFailed};

    Q_ENUM(UpdaterState)

    explicit AppUpdater(QObject* parent = nullptr);
    static AppUpdater* Instance();
    QString readyVersionStr() const;
    UpdaterState updaterState() const;

signals:
    void alreadyAtLatestVersion();
    void updaterStateChanged();
    void downloadFailed(QString error);
    void downloadProgress(qreal progress);

public slots:
    void checkForUpdates();
    void startUpdating();
    void downloadUpdate();

private:
    using LatestUpdateInfo = std::tuple<QString, unsigned int>;
    Promise<PlatformUpdateConfig> downloadConfigFile(QUrl link) const;
    Promise<QString> downloadAndSaveUpdate(QUrl link, QString localPath);
    Promise<QString> unzipUpdate(QString zipPath) const;
    Promise<QString> saveFile(QByteArray rawData, QString filePath) const;
    QString locateUpdaterExecutable(QString packagePath);
    LatestUpdateInfo latestPreparedUpdateInfo() const;
    bool verifyLocalUpdateCache(LatestUpdateInfo info);
    bool checkVersion(unsigned int theirVersion) const;
    void setIsUpdateReady(bool value);
    void setUpdaterState(const UpdaterState state);
    void saveLatestInfo(LatestUpdateInfo info);

private:
    UpdaterState _updaterState{ UpdaterState::NotReady };
    QThreadPool _workerPool;
    PlatformUpdateConfig _config;
};

#endif // APPUPDATER_HPP
