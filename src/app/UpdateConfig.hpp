#ifndef UPDATECONFIG_HPP
#define UPDATECONFIG_HPP

#include <QJsonObject>
#include <QStringList>
#include <QUrl>
#include <map>

struct PlatformUpdateConfig {
    PlatformUpdateConfig(unsigned int versionIn, QString nameIn, QString updateZipURLIn,
        QString updateDeltaZipUrlIn, QStringList removeFilesListIn, QString checksumHexIn);
    static PlatformUpdateConfig FromJson(QJsonObject obj);
    PlatformUpdateConfig() = default;
    PlatformUpdateConfig(const PlatformUpdateConfig &config);

    unsigned int version;
    QString name;
    QString updateZipURL;
    QString updateDeltaZipUrl;
    QStringList removeFilesList;
    QString checksumHex;
};

QUrl RemoteLatestUpdateConfigUrl(bool staging);
QUrl RemoteUpdateZipUrl(const PlatformUpdateConfig& config, bool staging);

#endif // UPDATECONFIG_HPP
