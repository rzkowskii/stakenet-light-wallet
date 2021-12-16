#ifndef ASSETSETTINGS_HPP
#define ASSETSETTINGS_HPP

#include "CoinAsset.hpp"
#include <QDataStream>
#include <QObject>
#include <QSet>
#include <QSettings>
#include <Tools/Common.hpp>
#include <map>
#include <vector>

struct AutopilotDetails {
    double allocation{ 0.0 };
    unsigned maxChannels{ 0 };

    AutopilotDetails() {}
    AutopilotDetails(double allocation, unsigned maxChannels);

    bool operator==(const AutopilotDetails& rhs) const;
    bool operator!=(const AutopilotDetails& rhs) const;
};

class AssetSettings : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool active READ isActive WRITE setIsActive NOTIFY activeChanged)
    Q_PROPERTY(bool autopilotActive READ isAutopilotActive WRITE setAutopilotActive NOTIFY
            autopilotActiveChanged)
    Q_PROPERTY(unsigned satsPerByte READ satsPerByte WRITE setSatsPerByte NOTIFY satsPerByteChanged)

public:
    struct Settings {
        unsigned assetID;
        bool active{ false };
        bool autopilotActive{ true };
        AutopilotDetails autopilotDetails;
        unsigned satsPerByte{ 0 };
        QString backupDir;

        bool operator==(const Settings& rhs) const;
    };

    explicit AssetSettings(unsigned assetID, bool active, bool autopilotActive,
        AutopilotDetails autopilotDetails, unsigned satsPerByte, QObject* parent = nullptr);

    AssetID assetID() const;

    bool isActive() const;
    bool isAutopilotActive() const;

    void setIsActive(bool value);
    void setAutopilotActive(bool value);

    unsigned satsPerByte() const;
    void setSatsPerByte(unsigned value);

    AutopilotDetails autopilotDetails() const;
    void setAutopilotDetails(AutopilotDetails autopilotDetails);

    QString backupDir() const;
    void setBackupDir(QString backupDir);

    QVariant save() const;
    void load(Settings settings);

signals:
    void activeChanged(bool newValue);
    void autopilotActiveChanged(bool newValue);
    void satsPerByteChanged(unsigned newValue);
    void backupDirChanged(QString newBackupDir);
    void autopilotDetailsChanged(AutopilotDetails autopilotDetails);

private:
    Settings _settings;
};

Q_DECLARE_METATYPE(AssetSettings::Settings)

QDataStream& operator<<(QDataStream& out, const AssetSettings::Settings& v);
QDataStream& operator>>(QDataStream& in, AssetSettings::Settings& v);

QDataStream& operator<<(QDataStream& out, const AutopilotDetails& v);
QDataStream& operator>>(QDataStream& in, AutopilotDetails& v);

#endif // ASSETSETTINGS_HPP
