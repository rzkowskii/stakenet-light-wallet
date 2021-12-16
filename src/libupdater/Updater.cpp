#include "Updater.hpp"
#include <Networking/NetworkingUtils.hpp>
#include <UpdaterUtils.hpp>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <Utils/Logging.hpp>

namespace {

//==============================================================================

const QLatin1String UPDATER_TMP_EXTENSION(".ciphr-updater-tmp");

//==============================================================================

QString SafeCopyFile(QString from, QString to)
{
    qDebug() << "Copying" << from << "to" << to;
    QString renamedTo;
    if (QFile::exists(to)) {
        QFile::remove(to + UPDATER_TMP_EXTENSION);
        if (!QFile::rename(to, to + UPDATER_TMP_EXTENSION)) {
            throw std::runtime_error("Failed to rename: " + to.toStdString());
        }

        renamedTo = to + UPDATER_TMP_EXTENSION;
    }

    if (!QFile::copy(from, to)) {
        throw std::runtime_error("Copy " + from.toStdString() + " " + to.toStdString() + " failed");
    }

    return renamedTo;
}

//==============================================================================

void SafeMergeDirectories(QString from, QString to, QList<QPair<QString, QString>>& renamedFiles)
{
    QDir fromDir(from);
    QDir toDir(to);

    if (!toDir.exists()) {
        toDir.mkpath(".");
    }

    qDebug() << "Merging" << from << "to" << to;
    for (const auto& entry : UpdaterUtils::EntryInfoList(fromDir)) {
        qDebug() << "Entry:" << entry.fileName();
        if (entry.isDir()) {
            SafeMergeDirectories(
                entry.absoluteFilePath(), toDir.absoluteFilePath(entry.fileName()), renamedFiles);
        } else if (entry.isFile()) {
            auto to = toDir.absoluteFilePath(entry.fileName());
            auto renamedPath = SafeCopyFile(entry.absoluteFilePath(), to);
            if (!renamedPath.isEmpty()) {
                renamedFiles.push_back(qMakePair(to, renamedPath));
            }
        }
    }
}
} // namespace

//==============================================================================

Updater::Updater(bool isStaging, QString updateJsonPath, QObject* parent)
    : QObject(parent)
    , _updateJsonPath(updateJsonPath)
    , _updateConfig(PlatformUpdateConfig::FromJson({}))
    , _isStaging(isStaging)
{
}

//==============================================================================

void Updater::startUpdate(QString installDir, QString packageDir)
{
    updateStarted();
    Cleanup(installDir);

    LogCDebug(General) << "Updating dir:" << installDir << "from:" << packageDir;

    loadScriptFile()
        .then([this, packageDir] {
            if (!UpdaterUtils::VerifyDataIntegrity(packageDir, _updateConfig.checksumHex)) {
                LogCDebug(General) << "Failed to verify integrity data" << packageDir;
                throw std::runtime_error("Failed to verify data integrity");
            }
        })
        .then([=] {
            removeFiles(installDir);
            mergeDirectories(installDir, packageDir);
        })
        .finally([this, installDir, packageDir] {
            postCleanup(installDir, packageDir);
            updateFinised();
        });
}

//==============================================================================

Updater::Promise<void> Updater::loadScriptFile()
{
    auto promise = _updateJsonPath.isEmpty()
        ? readRemoteJsonFile(RemoteLatestUpdateConfigUrl(_isStaging))
        : readLocalJsonFile(_updateJsonPath);

    return promise.then(
        [this](QJsonObject result) { _updateConfig = PlatformUpdateConfig::FromJson(result); });
}

//==============================================================================

Updater::Promise<QJsonObject> Updater::readLocalJsonFile(QString scriptFilePath) const
{
    return Promise<QJsonObject>([scriptFilePath](const auto& resolve, const auto&) {
        QFile file(scriptFilePath);
        if (file.open(QFile::ReadOnly)) {
            QJsonParseError error;
            auto doc = QJsonDocument::fromJson(file.readAll(), &error);
            if (error.error != QJsonParseError::NoError) {
                throw std::runtime_error(QString("Update file parsing error: %1")
                                             .arg(error.errorString())
                                             .toStdString());
            }

            resolve(doc.object());
        } else {
            throw std::runtime_error(
                QString("Can't open update conf, path:").arg(scriptFilePath).toStdString());
        }
    });
}

//==============================================================================

Updater::Promise<QJsonObject> Updater::readRemoteJsonFile(QUrl url) const
{
    return NetworkUtils::downloadFile(url).then(
        [](QByteArray rawData) { return QJsonDocument::fromJson(rawData).object(); });
}

//==============================================================================

void Updater::removeFiles(QString rootPath) const
{
    for (auto&& fileName : _updateConfig.removeFilesList) {
        QDir rootDir(rootPath);
        QFileInfo info(rootDir.absoluteFilePath(fileName));
        if (info.exists()) {
            if (info.isDir()) {
                info.absoluteDir().removeRecursively();
            } else if (info.isFile()) {
                QFile::remove(info.absoluteFilePath());
            }
        }
    }
}

//==============================================================================

void Updater::postCleanup(QString installPath, QString packagePath) const
{
    QDir packageDir(packagePath);
    LogCDebug(General) << "Post cleanup, removing package:" << packagePath
                       << packageDir.removeRecursively();
    Cleanup(installPath);
}

//==============================================================================

void Updater::Cleanup(QString installDir)
{
    QDir appDir(installDir);

    std::function<void(QDir)> helper = [&helper](QDir dir) {
        for (const auto& entry : UpdaterUtils::EntryInfoList(dir, false)) {
            if (entry.isDir()) {
                helper(QDir(entry.absoluteFilePath()));
            } else if (entry.isFile() && entry.absoluteFilePath().endsWith(UPDATER_TMP_EXTENSION)) {
                if (!QFile::remove(entry.absoluteFilePath())) {
                    LogCDebug(General) << "Updater::cleanup:"
                                       << "failed to remove" << entry.absoluteFilePath();
                }
            }
        }
    };

    helper(appDir);
}

//==============================================================================

void Updater::mergeDirectories(QString installPath, QString packagePath) const
{
    QDir updateDir(packagePath);
    QList<QPair<QString, QString>> renamedFiles; // { OriginalPath, RenamedPath }

    try {
        if (updateDir.exists()) {
            SafeMergeDirectories(updateDir.absolutePath(), installPath, renamedFiles);
        }
    } catch (std::exception&) {
        // restore to old state what was affected
        for (const auto& entry : renamedFiles) {
            QFile::rename(entry.second, entry.first);
        }
        throw;
    }
}

//==============================================================================
