#include "AppUpdater.hpp"
#include <Networking/NetworkingUtils.hpp>
#include <StakenetConfig.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QtConcurrent>

#ifndef Q_OS_ANDROID
#include <JlCompress.h>
#endif

//==============================================================================

class UnzipFailedException : public QException {
public:
    void raise() const override { throw *this; }
    UnzipFailedException* clone() const override { return new UnzipFailedException(*this); }
};

//==============================================================================

static const QString zipName("data.zip");
static const QString SETTINGS_DOWNLOADED_UPDATE_PATH("SETTINGS_DOWNLOADED_UPDATE_PATH");
static const QString SETTINGS_DOWNLOADED_UPDATE_VERSION("SETTINGS_DOWNLOADED_UPDATE_VERSION");
static const QString tempUpdaterDir("stakenet-wallet-updater");

//==============================================================================

static QString updaterExecutableName()
{
#ifdef Q_OS_WIN
    return QString("updater.exe");
#else
    return QString("updater");
#endif
}

//==============================================================================

static QDir GetPackageDir()
{
#ifdef Q_OS_OSX
    QDir macOSDir(QCoreApplication::applicationDirPath());
    macOSDir.cdUp();
    macOSDir.cdUp();
    return macOSDir;
#elif defined(Q_OS_WIN)
    return QDir(QCoreApplication::applicationDirPath());
#else
    QDir linuxDir(QCoreApplication::applicationDirPath());
    linuxDir.cdUp();
    return linuxDir;
#endif
    return QDir();
}

//==============================================================================

static QDir BuildLocalPathForUpdate()
{
    QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    const QString dirPath("stakenet-wallet/update");
    if (tempDir.exists(dirPath)) {
        tempDir.rmdir(dirPath);
    }

    tempDir.mkpath(dirPath);
    tempDir.cd(dirPath);

    LogCDebug(General) << tempDir.absolutePath();
    return tempDir;
}

//==============================================================================

AppUpdater::AppUpdater(QObject* parent)
    : QObject(parent)
{
    _workerPool.setMaxThreadCount(1);
}

//==============================================================================

AppUpdater* AppUpdater::Instance()
{
    static AppUpdater instance;
    return &instance;
}

//==============================================================================

QString AppUpdater::readyVersionStr() const
{
    auto numericVersion = std::get<1>(latestPreparedUpdateInfo());

    std::vector<unsigned int> helper{ 1000000, 10000, 100, 1 };

    QString strVersion("%1.%2.%3.%4");
    for (auto&& item : helper) {
        strVersion = strVersion.arg(numericVersion / item);
        numericVersion %= item;
    }

    return strVersion;
}

//==============================================================================

AppUpdater::UpdaterState AppUpdater::updaterState() const
{
    return _updaterState;
}

//==============================================================================

void AppUpdater::checkForUpdates()
{
    if (_updaterState != UpdaterState::NotReady && _updaterState != UpdaterState::CheckingFailed)
        return;

    setUpdaterState(UpdaterState::Checking);

    downloadConfigFile(RemoteLatestUpdateConfigUrl(ApplicationViewModel::IsStagingEnv()))
        .then([this](PlatformUpdateConfig config) {
            _config = config;
            if (!checkVersion(config.version)) {
                setUpdaterState(UpdaterState::NotReady);
                this->alreadyAtLatestVersion();
                verifyLocalUpdateCache(latestPreparedUpdateInfo());
                return;
            }
            auto latestUpdateInfo = latestPreparedUpdateInfo();
            if (std::get<1>(latestUpdateInfo) == _config.version
                && verifyLocalUpdateCache(latestUpdateInfo)) {
                saveLatestInfo(latestUpdateInfo);
                verifyLocalUpdateCache(latestPreparedUpdateInfo());
                return;
            }
             setUpdaterState(UpdaterState::ExistNew);

        }).fail([this](const QString& error) {

        setUpdaterState(UpdaterState::CheckingFailed);
        downloadFailed(error);
    });
}

//==============================================================================

void AppUpdater::startUpdating()
{
    auto updateInfo = latestPreparedUpdateInfo();
    if (!verifyLocalUpdateCache(updateInfo)) {
        return;
    }

    // for now don't use delta updates, we will download always full updates.
    bool canDeltaUpdate = false; //(std::get<1>(updateInfo) -
                                 // ApplicationViewModel::Instance()->numericVersion()) == 1;

    auto updaterPath = locateUpdaterExecutable(std::get<0>(updateInfo));

    if (!QFileInfo::exists(updaterPath)) {
        downloadFailed("Failed to locate updater executable");
        QDir(std::get<0>(updateInfo)).removeRecursively();
        checkForUpdates();
        return;
    }

    QDir tempDir(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    if (tempDir.exists(tempUpdaterDir)) {
        tempDir.cd(tempUpdaterDir);
        tempDir.removeRecursively();
        tempDir.mkpath(".");
    } else {
        tempDir.mkdir(tempUpdaterDir);
        tempDir.cd(tempUpdaterDir);
    }

    auto program = tempDir.absoluteFilePath(QFileInfo(updaterPath).fileName());
    QFile::copy(updaterPath, program);

    QStringList arguments;
    arguments << "--install-dir" << GetPackageDir().absolutePath() << "--package-dir"
              << std::get<0>(updateInfo) << "--post-update-event"
              << QCoreApplication::applicationFilePath();

    if (ApplicationViewModel::IsStagingEnv()) {
        arguments << "--staging";
    }

    if (canDeltaUpdate) {
        arguments << "--delta-update";
    }

    QVariantMap payload;
    payload["program"] = program;
    payload["args"] = arguments;
    QCoreApplication::instance()->setProperty("pending_update", payload);

    QCoreApplication::quit();
}

//==============================================================================

void AppUpdater::downloadUpdate()
{
    if (_updaterState != UpdaterState::ExistNew && _updaterState != UpdaterState::DownloadingFailed)
        return;

    setUpdaterState(UpdaterState::Downloading);
    downloadAndSaveUpdate(RemoteUpdateZipUrl(_config, ApplicationViewModel::IsStagingEnv()),
        BuildLocalPathForUpdate().absoluteFilePath(zipName))
        .then([this](QString localPath) {
            return unzipUpdate(localPath).then(
                [=](QString path) {
                saveLatestInfo(std::make_tuple(path, _config.version));
                verifyLocalUpdateCache(latestPreparedUpdateInfo());});
        })
        .fail([this](const QString& error) {
        QDir dir(BuildLocalPathForUpdate().absolutePath());
        dir.removeRecursively();
        setUpdaterState(UpdaterState::DownloadingFailed);
        downloadFailed(error);
    });
}

//==============================================================================

Promise<PlatformUpdateConfig> AppUpdater::downloadConfigFile(QUrl link) const
{
    return NetworkUtils::downloadFile(link).then([](QByteArray rawData) {
        return PlatformUpdateConfig::FromJson(QJsonDocument::fromJson(rawData).object());
    });
}

//==============================================================================

Promise<QString> AppUpdater::downloadAndSaveUpdate(QUrl link, QString localPath)
{
    auto progressHandler = [this](auto current, auto max) {
        this->downloadProgress(static_cast<qreal>(current) / static_cast<qreal>(max));
    };
    return NetworkUtils::downloadFile(link, progressHandler)
        .then([this, localPath](QByteArray rawData) {
            return QtConcurrent::run([=] {
                saveFile(rawData, localPath);
                return localPath;
            });
        });
}

//==============================================================================

Promise<QString> AppUpdater::unzipUpdate(QString zipPath) const
{
    return QtPromise::resolve(QtConcurrent::run([zipPath]() {
        QFileInfo info(zipPath);
        QDir unzipDir = info.absoluteDir();
        QString zipDir("uncompressed");
        if (unzipDir.exists(zipDir)) {
            QDir(unzipDir.absoluteFilePath(zipDir)).removeRecursively();
        }
        unzipDir.mkdir(zipDir);
        auto targetDirPath = unzipDir.absoluteFilePath(zipDir);
#if defined(Q_OS_LINUX)
        QProcess::execute("unzip", QStringList() << "-qq" << zipPath << "-d" << targetDirPath);
        return targetDirPath;
#elif defined(Q_OS_MAC)
        QStringList arguments;
        arguments << "-xk" << zipPath << targetDirPath;
        QProcess::execute("ditto", arguments);
        return targetDirPath;
#else
        if (!JlCompress::extractDir(zipPath, targetDirPath).isEmpty()) {
            return targetDirPath;
        } else {
            throw UnzipFailedException();
        }
#endif
    }));
}

//==============================================================================

Promise<QString> AppUpdater::saveFile(QByteArray rawData, QString filePath) const
{
    return Promise<QString>([rawData, filePath](const auto& resolve, const auto& reject) {
        QFile file(filePath);
        if (file.open(QIODevice::WriteOnly | QFile::Truncate)) {
            file.write(rawData);
            resolve(filePath);
        } else {
            reject();
        }
    });
}

//==============================================================================

QString AppUpdater::locateUpdaterExecutable(QString packagePath)
{
    QDir dir(packagePath);
#ifdef Q_OS_OSX
    dir.cd("Contents/MacOS");
#elif defined(Q_OS_LINUX)
    dir.cd("bin");
#endif

    return dir.absoluteFilePath(updaterExecutableName());
}

//==============================================================================

AppUpdater::LatestUpdateInfo AppUpdater::latestPreparedUpdateInfo() const
{
    QSettings settings;
    return std::make_tuple(settings.value(SETTINGS_DOWNLOADED_UPDATE_PATH).toString(),
        settings.value(SETTINGS_DOWNLOADED_UPDATE_VERSION).toInt());
}

//==============================================================================

bool AppUpdater::verifyLocalUpdateCache(LatestUpdateInfo info)
{
    QDir updateDir(std::get<0>(info));

    auto checkDir = [&info, &updateDir] {
        return !std::get<0>(info).isEmpty() && updateDir.exists()
            && !updateDir.entryList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot).isEmpty();
    };

    if (checkVersion(std::get<1>(info)) && checkDir()) {
        setUpdaterState(UpdaterState::Ready);
        return true;
    }

    setUpdaterState(UpdaterState::NotReady);
    return false;
}

//==============================================================================

bool AppUpdater::checkVersion(unsigned int theirVersion) const
{
    return StakenetNumericVersion() < theirVersion;
}

//==============================================================================

void AppUpdater::setUpdaterState(const AppUpdater::UpdaterState state)
{
    if (_updaterState != state) {
        _updaterState = state;
        if (_updaterState == UpdaterState::NotReady) {
            QSettings settings;
            settings.remove(SETTINGS_DOWNLOADED_UPDATE_PATH);
            settings.remove(SETTINGS_DOWNLOADED_UPDATE_VERSION);
        }
        emit updaterStateChanged();
    }
}

//==============================================================================

void AppUpdater::saveLatestInfo(AppUpdater::LatestUpdateInfo info)
{
    if (std::get<0>(info).isEmpty()) {
       return;
    }

    QSettings settings;
    settings.setValue(SETTINGS_DOWNLOADED_UPDATE_PATH, std::get<0>(info));
    settings.setValue(SETTINGS_DOWNLOADED_UPDATE_VERSION, std::get<1>(info));
    settings.sync();
}

//==============================================================================
