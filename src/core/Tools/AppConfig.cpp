#include "AppConfig.hpp"

#include <Data/AssetSettings.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <LndTools/LndTypes.hpp>
#include <Utils/Logging.hpp>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>

//==============================================================================

#define MAKE_KEY(name) static QLatin1String name(#name);

namespace Keys {
namespace Lnd {
    MAKE_KEY(dataDir)
    MAKE_KEY(resolverpath)
    MAKE_KEY(configs)
}
namespace Connext {
    MAKE_KEY(dataDir)
    MAKE_KEY(resolverHost)
    MAKE_KEY(resolverPort)
    MAKE_KEY(clientConfig)
}
namespace ConnextDaemonConfig {
    MAKE_KEY(host)
    MAKE_KEY(port)
}

namespace DaemonConfig {
    MAKE_KEY(asset)
    MAKE_KEY(rpclisten)
    MAKE_KEY(restlisten)
    MAKE_KEY(listen)
    MAKE_KEY(chain)
    MAKE_KEY(tlsCert)
    MAKE_KEY(tlsKey)
    MAKE_KEY(macaroonPath)
    MAKE_KEY(zmqport)
    MAKE_KEY(cltvExpiry)
    MAKE_KEY(maxChanSizeSat)
    MAKE_KEY(hostList)
    MAKE_KEY(watchTowerList)
    MAKE_KEY(autopilotAllocation)
    MAKE_KEY(autopilotMaxChannels)
    MAKE_KEY(autopilotFeeRate)
}
MAKE_KEY(connextConfig)
MAKE_KEY(lndConfig)
MAKE_KEY(dataDir)
MAKE_KEY(skinPath)
MAKE_KEY(rpcPort)
}

//==============================================================================

const std::map<AssetID, DaemonConfig> defaultDeamonCfg{ { 0,
        DaemonConfig("btc", "127.0.0.1:20000",
                     "0.0.0.0:8000", "9000", "bitcoin",
                     QString(), // "tls.cert"
                     QString(), // "tls.key"
                     QString(), // macaroonPath
                     23455,
                     2016, // cltv expiry
                     "130000000", // MaxAutopilotChanSize in satoshis
        { { "03757b80302c8dfe38a127c252700ec3052e5168a7ec6ba183cdab2ac7adad3910@178.128.97.48:11000" }
          /*, { "02bfe54c7b2ce6f737f0074062a2f2aaf855f81741474c05fd4836a33595960e18@178.128.97.48:21000" }*/ },
        {"03274679a30f621526fabbcc014dbfb2ed50b697f31fe1a083aa39f26b20ddc314@178.128.97.48:19000"}, // watchtower peer
                     0.0, 0, 0) },

    { 2,
        DaemonConfig("ltc", "127.0.0.1:20001", "0.0.0.0:8002", "9001", "litecoin",
                     QString(), // "tls.cert"
                     QString(), // "tls.key"
                     QString(), // macaroonPath
                     23457,
                     4032, // cltv expiry
                     "20000000000", // MaxAutopilotChanSize in satoshis
        { { "0375e7d882b442785aa697d57c3ed3aef523eb2743193389bd205f9ae0c609e6f3@178.128.97.48:11002" },
          /*{ "0211eeda84950d7078aa62383c7b91def5cf6c5bb52d209a324cda0482dbfbe4d2@178.128.97.48:21002" }*/ },
        {"02907fca5ba5b0115d15ab03349ab5ba86fed82876be041037abe4a60b535516f4@178.128.97.48:19002"}, // watchtower peer
                     0.0, 0, 0) },

    { 384,
        DaemonConfig("xsn", "127.0.0.1:20002", "0.0.0.0:8384", "9002", "xsncoin",
                    QString(), // "tls.cert"
                    QString(), // "tls.key"
                    QString(), // macaroonPath
                    23456,
                    10080, // cltv expiry
                    "20000000000000", // MaxAutopilotChanSize in satoshis
        { { "0396ca2f7cec03d3d179464acd57b4e6eabebb5f201705fa56e83363e3ccc622bb@178.128.97.48:11384" },
          /* { "03bc3a97ffad197796fc2ea99fc63131b2fd6158992f174860c696af9f215b5cf1@178.128.97.48:21384" } */},
        {"03d1fe0c7ca70208f7884780efa8baa0419d4b14704bf5c8bfd0c703c5fd204f08@178.128.97.48:19384"}, // watchtower peer
                     0.0, 0, 0) } };

const AppConfig::ConnextConfig defaultConnextConfig("", "192.168.0.1", 1235,
    ConnextDaemonConfig(
        "http://127.0.0.1", 8001, "vector6At5HhbfhcE1p1SBTxAL5ej71AGWkSSxrSTmdj6wuFHquJ1hg8"));

//==============================================================================

static QJsonObject CreateDefaultLndCfg()
{
    QJsonObject obj;
    obj.insert(Keys::Lnd::dataDir, "exchange-b");
    obj.insert(Keys::Lnd::resolverpath, "");
    obj.insert(Keys::Lnd::configs, QJsonArray());
    return obj;
}

//==============================================================================

static DaemonConfig DaemonConfigFromJson(QJsonObject obj)
{
    auto pull = [&obj](auto name) { return obj.value(name).toString(); };

    QStringList hostConf;
    auto peerArray = obj.value(Keys::DaemonConfig::hostList).toArray();

    for (auto peer : peerArray) {
        hostConf.append(peer.toString());
    }

    QStringList watchTowerConf;
    auto towerPeersArray = obj.value(Keys::DaemonConfig::watchTowerList).toArray();

    for (auto peer : towerPeersArray) {
        watchTowerConf.append(peer.toString());
    }

    return DaemonConfig(pull(Keys::DaemonConfig::asset), pull(Keys::DaemonConfig::rpclisten),
        pull(Keys::DaemonConfig::listen), pull(Keys::DaemonConfig::restlisten),
        pull(Keys::DaemonConfig::chain), pull(Keys::DaemonConfig::tlsCert),
        pull(Keys::DaemonConfig::tlsKey), pull(Keys::DaemonConfig::macaroonPath),
        obj.value(Keys::DaemonConfig::zmqport).toInt(),
        obj.value(Keys::DaemonConfig::cltvExpiry).toInt(), pull(Keys::DaemonConfig::maxChanSizeSat),
        hostConf, watchTowerConf, obj.value(Keys::DaemonConfig::autopilotAllocation).toDouble(),
        static_cast<unsigned>(obj.value(Keys::DaemonConfig::autopilotMaxChannels).toInt()),
        static_cast<unsigned>(obj.value(Keys::DaemonConfig::autopilotFeeRate).toInt()));
}

//==============================================================================

AppConfig& AppConfig::Instance()
{
    static AppConfig instance;
    return instance;
}

//==============================================================================

void AppConfig::fillAutopilotArg(DaemonConfig& deamonConfig, const WalletAssetsModel& assetsModel)
{
    auto asset = assetsModel.assetByName(deamonConfig.assetSymbol);
    auto assetSettings = assetsModel.assetSettings(asset.coinID());

    auto savedSatsPerByte = assetSettings->satsPerByte();
    auto savedAllocation = assetSettings->autopilotDetails().allocation;
    auto savedMaxChannels = assetSettings->autopilotDetails().maxChannels;

    deamonConfig.autopilotAllocation
        = savedAllocation > 0.0 ? savedAllocation : asset.lndData().autopilotDefaultAllocation;
    deamonConfig.autopilotMaxChannels
        = savedMaxChannels > 0 ? savedMaxChannels : asset.lndData().autopilotDefaultMaxChannels;
    deamonConfig.autopilotFeeRate
        = savedSatsPerByte > 0 ? savedSatsPerByte : asset.lndData().defaultSatsPerByte;
}

//==============================================================================

void AppConfig::parse(const WalletAssetsModel& assetsModel, QString fromFile)
{
    QFile file(fromFile);

    QJsonObject obj;
    if (file.open(QFile::ReadOnly)) {
        QJsonParseError error;
        obj = QJsonDocument::fromJson(file.readAll(), &error).object();
        if (obj.isEmpty()) {
            LogCDebug(General) << "PARSING ERROR ===" << error.errorString();
        }
    } else if (!fromFile.isEmpty()) {
        throw std::runtime_error("Failed to open config");
    }

    _rootObject = validate(assetsModel, obj);
    _config = Config::FromJson(obj, assetsModel);

    initDefaultLndConfig(assetsModel);
    initDefaultConnextConfig();
}

//==============================================================================

const AppConfig::Config& AppConfig::config() const
{
    return _config;
}

//==============================================================================

AppConfig::AppConfig(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

QJsonObject AppConfig::validate(const WalletAssetsModel& assetsModel, QJsonObject rootObject)
{
    QJsonObject validated = rootObject;
    auto validateKey = [](const QJsonObject& obj, QLatin1String key) {
        if (!obj.contains(key)) {
            throw std::runtime_error(std::string("Missing key: ") + key.data());
        }
    };

    auto validateKeys = [&validateKey](const QJsonObject& obj,
                            std::initializer_list<QLatin1String>&& testVector) {
        for (auto&& test : testVector) {
            validateKey(obj, test);
        }
    };

    auto validateDaemonCfg = [&validateKeys, &assetsModel](QJsonObject obj) {
        validateKeys(obj,
            { Keys::DaemonConfig::asset, Keys::DaemonConfig::listen, Keys::DaemonConfig::restlisten,
                Keys::DaemonConfig::rpclisten, Keys::DaemonConfig::chain,
                Keys::DaemonConfig::zmqport, Keys::DaemonConfig::cltvExpiry,
                Keys::DaemonConfig::maxChanSizeSat, Keys::DaemonConfig::hostList,
                Keys::DaemonConfig::autopilotAllocation, Keys::DaemonConfig::autopilotMaxChannels,
                Keys::DaemonConfig::autopilotFeeRate });

        auto assetName = obj.value(Keys::DaemonConfig::asset).toString();
        if (!assetsModel.hasAsset(assetName)) {
            throw std::runtime_error("Invalid assetName: " + assetName.toStdString());
        }
    };

    auto validateLndCfg = [&validateKeys, validateDaemonCfg](QJsonObject obj) {
        validateKeys(obj, { Keys::Lnd::dataDir, Keys::Lnd::resolverpath });

        for (auto&& value : obj.value(Keys::Lnd::configs).toArray()) {
            validateDaemonCfg(value.toObject());
        }
    };

    if (validated.value(Keys::lndConfig).isNull()
        || validated.value(Keys::lndConfig).isUndefined()) {
        validated.insert(Keys::lndConfig, CreateDefaultLndCfg());
    }

    validateLndCfg(validated.value(Keys::lndConfig).toObject());

    return validated;
}

//==============================================================================

void AppConfig::initDefaultLndConfig(const WalletAssetsModel& assetsModel)
{
    for (auto&& it : defaultDeamonCfg) {
        auto& lndConfigs = _config.lndConfig.lndsConfig;
        if (lndConfigs.count(it.first) == 0) {
            lndConfigs.emplace(it.first, it.second);
            fillAutopilotArg(lndConfigs.at(it.first), assetsModel);
        }
    }
}

//==============================================================================

void AppConfig::initDefaultConnextConfig()
{
    _config.connextConfig = defaultConnextConfig;
}

//==============================================================================

AppConfig::Config AppConfig::Config::FromJson(QJsonObject obj, const WalletAssetsModel& assetsModel)
{
    Config config;
    config.dataDirPath = obj.value(Keys::dataDir).toString();
    config.skinPath = obj.value(Keys::skinPath).toString();
    config.rpcPort = obj.value(Keys::rpcPort).toInt(12345);
    config.lndConfig = LndConfig::FromJson(obj.value(Keys::lndConfig).toObject(), assetsModel);
    config.connextConfig = ConnextConfig::FromJson(obj.value(Keys::connextConfig).toObject());

    return config;
}

//==============================================================================

const DaemonConfig& AppConfig::LndConfig::daemonConfigById(AssetID assetID) const
{
    return lndsConfig.at(assetID);
}

//==============================================================================

AppConfig::LndConfig AppConfig::LndConfig::FromJson(
    QJsonObject obj, const WalletAssetsModel& assetsModel)
{
    LndConfig config;

    QJsonArray lndsConfig = obj.value(Keys::Lnd::configs).toArray();

    for (QJsonValue value : lndsConfig) {
        QJsonObject daemonConfig = value.toObject();
        QString assetSymbol = daemonConfig.value(Keys::DaemonConfig::asset).toString();
        auto assetId = assetsModel.assetByName(assetSymbol).coinID();
        config.lndsConfig.emplace(assetId, DaemonConfigFromJson(daemonConfig));
    }

    config.dataDirPath = obj.value(Keys::Lnd::dataDir).toString();
    config.resolverPath = obj.value(Keys::Lnd::resolverpath).toString();

    return config;
}

//==============================================================================

AppConfig::ConnextConfig AppConfig::ConnextConfig::FromJson(QJsonObject obj)
{
    ConnextConfig config;

    auto clientConfigObj = obj.value(Keys::Connext::clientConfig).toObject();
    config.clientConfig.host = clientConfigObj.value(Keys::ConnextDaemonConfig::host).toString();
    config.clientConfig.port = clientConfigObj.value(Keys::ConnextDaemonConfig::port).toInt();

    config.resolverHost = obj.value(Keys::Connext::resolverHost).toString();
    config.resolverPort = obj.value(Keys::Connext::resolverPort).toInt();
    config.dataDirPath = obj.value(Keys::Connext::dataDir).toString();

    return config;
}

//==============================================================================
