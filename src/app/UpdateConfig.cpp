#include "UpdateConfig.hpp"

#include <QJsonArray>
#include <functional>

//==============================================================================

static QString GetPlatformSuffix()
{
    QString platformName;
#ifdef Q_OS_OSX
    platformName = "osx";
#elif defined(Q_OS_WIN)
    platformName = "win";
#else
    platformName = "linux";
#endif

    return platformName;
}

//==============================================================================

static QString GetPlatformBucketUrl(bool isStaging)
{
    QString envSuffix(isStaging ? "staging" : "production");
    return QString("https://wallet.stakenet.io/light-wallet/%1/%2")
        .arg(envSuffix)
        .arg(GetPlatformSuffix());
}

//==============================================================================

PlatformUpdateConfig::PlatformUpdateConfig(unsigned int versionIn, QString nameIn,
    QString updateZipURLIn, QString updateDeltaZipUrlIn, QStringList removeFilesListIn,
    QString checksumHexIn)
    : version(versionIn)
    , name(nameIn)
    , updateZipURL(updateZipURLIn)
    , updateDeltaZipUrl(updateDeltaZipUrlIn)
    , removeFilesList(removeFilesListIn)
    , checksumHex(checksumHexIn)
{
}

//==============================================================================

PlatformUpdateConfig::PlatformUpdateConfig(const PlatformUpdateConfig &config)
    : version(config.version)
    , name(config.name)
    , updateZipURL(config.updateZipURL)
    , updateDeltaZipUrl(config.updateDeltaZipUrl)
    , removeFilesList(config.removeFilesList)
    , checksumHex(config.checksumHex)
{
}

//==============================================================================

PlatformUpdateConfig PlatformUpdateConfig::FromJson(QJsonObject obj)
{
    QStringList removeList;
    auto arr = obj.value("remove").toArray();
    std::transform(std::begin(arr), std::end(arr), std::back_inserter(removeList),
        [](const auto& val) { return val.toString(); });

    return PlatformUpdateConfig(obj.value("version").toInt(), obj.value("name").toString(),
        obj.value("url").toString(), obj.value("url-delta").toString(), removeList,
        obj.value("checksum").toString());
}

//==============================================================================

QUrl RemoteLatestUpdateConfigUrl(bool staging)
{
    return QUrl(GetPlatformBucketUrl(staging) + "/update.json");
}

//==============================================================================

QUrl RemoteUpdateZipUrl(const PlatformUpdateConfig& config, bool staging)
{
    if (config.updateZipURL.isEmpty()) {
        if (config.name.isEmpty()) {
            throw std::runtime_error("Invalid update file, `name` is missing");
        }

        return QString("%1/builds/%2").arg(GetPlatformBucketUrl(staging)).arg(config.name);
    }

    return config.updateZipURL;
}

//==============================================================================
