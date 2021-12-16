#include "AssetSettings.hpp"
#include <Utils/Logging.hpp>

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QVariant>

//==============================================================================

AssetSettings::AssetSettings(unsigned assetID, bool active, bool autopilotActive,
    AutopilotDetails autopilotDetails, unsigned satsPerByte, QObject* parent)
    : QObject(parent)
{
    _settings.assetID = assetID;
    _settings.active = active;
    _settings.autopilotActive = autopilotActive;
    _settings.autopilotDetails = autopilotDetails;
    _settings.satsPerByte = satsPerByte;
}

//==============================================================================

AssetID AssetSettings::assetID() const
{
    return _settings.assetID;
}

//==============================================================================

bool AssetSettings::Settings::operator==(const Settings& rhs) const
{
    return assetID == rhs.assetID;
}

//==============================================================================

bool AssetSettings::isActive() const
{
    return _settings.active;
}

//==============================================================================

bool AssetSettings::isAutopilotActive() const
{
    return _settings.autopilotActive;
}

//==============================================================================

void AssetSettings::setIsActive(bool value)
{
    if (_settings.active != value) {
        _settings.active = value;
        activeChanged(value);
    }
}

//==============================================================================

void AssetSettings::setAutopilotActive(bool value)
{
    if (_settings.autopilotActive != value) {
        _settings.autopilotActive = value;
        autopilotActiveChanged(value);
    }
}

//==============================================================================

unsigned AssetSettings::satsPerByte() const
{
    return _settings.satsPerByte;
}

//==============================================================================

void AssetSettings::setSatsPerByte(unsigned value)
{
    if (_settings.satsPerByte != value) {
        _settings.satsPerByte = value;
        satsPerByteChanged(value);
    }
}

//==============================================================================

AutopilotDetails AssetSettings::autopilotDetails() const
{
    return _settings.autopilotDetails;
}

//==============================================================================

void AssetSettings::setAutopilotDetails(AutopilotDetails autopilotDetails)
{
    if (_settings.autopilotDetails != autopilotDetails) {
        _settings.autopilotDetails.allocation = autopilotDetails.allocation;
        _settings.autopilotDetails.maxChannels = autopilotDetails.maxChannels;
        autopilotDetailsChanged(autopilotDetails);
    }
}

//==============================================================================

QString AssetSettings::backupDir() const
{
    return _settings.backupDir;
}

//==============================================================================

void AssetSettings::setBackupDir(QString backupDir)
{
    if (QFileInfo(backupDir).isDir() && _settings.backupDir != backupDir) {
        _settings.backupDir = backupDir;
        backupDirChanged(backupDir);
    }
}

//==============================================================================

QVariant AssetSettings::save() const
{
    return QVariant::fromValue(_settings);
}

//==============================================================================

void AssetSettings::load(AssetSettings::Settings settings)
{
    _settings = settings;
}

//==============================================================================

QDataStream& operator<<(QDataStream& out, const AssetSettings::Settings& settings)
{
    out << settings.assetID << settings.active << settings.autopilotActive
        << settings.autopilotDetails << settings.satsPerByte << settings.backupDir;

    return out;
}

//==============================================================================

QDataStream& operator>>(QDataStream& in, AssetSettings::Settings& settings)
{
    in >> settings.assetID;
    in >> settings.active;
    in >> settings.autopilotActive;
    in >> settings.autopilotDetails;
    in >> settings.satsPerByte;
    in >> settings.backupDir;
    return in;
}

//==============================================================================

QDataStream& operator<<(QDataStream& out, const AutopilotDetails& settings)
{
    out << settings.allocation << settings.maxChannels;

    return out;
}

//==============================================================================

QDataStream& operator>>(QDataStream& in, AutopilotDetails& settings)
{
    in >> settings.allocation;
    in >> settings.maxChannels;
    return in;
}

//==============================================================================

AutopilotDetails::AutopilotDetails(double allocation, unsigned maxChannels)
    : allocation(allocation)
    , maxChannels(maxChannels)
{
}

//==============================================================================

bool AutopilotDetails::operator==(const AutopilotDetails& rhs) const
{
    return allocation == rhs.allocation && maxChannels == rhs.maxChannels;
}

//==============================================================================

bool AutopilotDetails::operator!=(const AutopilotDetails& rhs) const
{
    return allocation != rhs.allocation || maxChannels != rhs.maxChannels;
}

//==============================================================================
