#ifndef APPCONFIG_HPP
#define APPCONFIG_HPP

#include <Data/WalletAssetsModel.hpp>
#include <LndTools/LndTypes.hpp>
#include <QJsonObject>
#include <QObject>
#include <Tools/Common.hpp>

class WalletAssetsModel;

class AppConfig : public QObject {
    Q_OBJECT
public:
    static AppConfig& Instance();

    void parse(const WalletAssetsModel& assetsModel, QString fromFile);
    void fillAutopilotArg(DaemonConfig& deamonConfig, const WalletAssetsModel& assetsModel);

    struct LndConfig {
        std::map<AssetID, DaemonConfig> lndsConfig;
        QString dataDirPath;
        QString resolverPath;

        const DaemonConfig& daemonConfigById(AssetID assetID) const;

        static LndConfig FromJson(QJsonObject obj, const WalletAssetsModel& assetsModel);
    };

    struct ConnextConfig {
        QString dataDirPath;
        QString resolverHost;
        int resolverPort;
        ConnextDaemonConfig clientConfig;
        ConnextConfig() {}

        ConnextConfig(QString path, QString host, int port, ConnextDaemonConfig client)
            : dataDirPath(path)
            , resolverHost(host)
            , resolverPort(port)
            , clientConfig(client)
        {
        }

        static ConnextConfig FromJson(QJsonObject obj);
    };

    struct Config {
        QString dataDirPath;
        QString skinPath;
        int rpcPort{ -1 };
        LndConfig lndConfig;
        ConnextConfig connextConfig;

        static Config FromJson(QJsonObject obj, const WalletAssetsModel& assetsModel);
    };

    const Config& config() const;

signals:

public slots:

private:
    explicit AppConfig(QObject* parent = nullptr);
    QJsonObject validate(const WalletAssetsModel& assetsModel, QJsonObject rootObject);
    void initDefaultLndConfig(const WalletAssetsModel& assetsModel);
    void initDefaultConnextConfig();

private:
    QJsonObject _rootObject;
    Config _config;
};

#endif // APPCONFIG_HPP
