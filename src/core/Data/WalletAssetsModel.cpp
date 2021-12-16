#include "WalletAssetsModel.hpp"
#include <Utils/Logging.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

//==============================================================================

static const QString SETTINGS_ACTIVE_ASSETS("activeAssets");

//==============================================================================

WalletAssetsModel::WalletAssetsModel(QString assetFilePath, QObject* parent)
    : QObject(parent)
{
    init(assetFilePath);
}

//==============================================================================

WalletAssetsModel::~WalletAssetsModel() {}

//==============================================================================

auto WalletAssetsModel::assets() const -> Assets
{
    Assets result;
    for (auto&& item : _assets)
        result.emplace_back(item.second);

    return result;
}

//==============================================================================

bool WalletAssetsModel::hasAsset(AssetID id) const
{
    return _assets.count(id) > 0;
}

//==============================================================================

bool WalletAssetsModel::hasAsset(QString name) const
{
    auto it = std::find_if(std::begin(_assets), std::end(_assets),
        [name](const auto& it) { return it.second.ticket().toLower() == name.toLower(); });

    return it != std::end(_assets);
}

//==============================================================================

const CoinAsset& WalletAssetsModel::assetById(unsigned id) const
{
    return _assets.at(id);
}

//==============================================================================

const CoinAsset& WalletAssetsModel::assetByName(QString name) const
{
    for (auto&& asset : _assets) {
        if (asset.second.ticket().toLower().contains(name.toLower())) {
            return asset.second;
        }
    }

    throw std::runtime_error("No asset with given name");
}

//==============================================================================

const CoinAsset& WalletAssetsModel::assetByContract(QString contract) const
{
    for (auto&& asset : _assets) {
        if (asset.second.type() == CoinAsset::Type::Account) {
            if (asset.second.connextData().tokenAddress == contract) {
                return asset.second;
            }
        }
    }

    throw std::runtime_error("No asset with given contract");
}

//==============================================================================

const std::vector<AssetID> WalletAssetsModel::activeAssets() const
{
    std::vector<AssetID> result;
    for (auto&& it : _activeAssets) {
        if (it.second->isActive()) {
            result.push_back(it.second->assetID());
        }
    }
    return result;
}

//==============================================================================

void WalletAssetsModel::init(QString assetFilePath)
{
    QFile file(assetFilePath);

    if (file.open(QFile::ReadOnly)) {
        auto doc = QJsonDocument::fromJson(file.readAll());
        for (QJsonValueRef object : doc.array()) {
            auto obj = object.toObject();
            if (obj.value("hidden").toBool()) {
                continue;
            }
            auto asset = CoinAsset::FromJson(obj);
            addAsset(asset);
            auto tokens = obj.value("tokens").toArray();

            for (auto tokenVal : tokens) {
                if (tokenVal.toObject().value("hidden").toBool()) {
                    continue;
                }
                addAsset(CoinAsset::TokenFromJson(asset, tokenVal.toObject()));
            }
        }
    } else {
        LogCCritical(General) << "Failed to open assets_conf.json";
        throw std::runtime_error("Failed to open assets_conf.json");
    }

#if 0
    LogCDebug(General) << "Read assets:";
    for(auto &&asset : assets())
    {
        LogCDebug(General) << "\t" << asset.coinID() << asset.name();
    }
    LogCDebug(General) << "End of read assets";
#endif

    qRegisterMetaTypeStreamOperators<AssetSettings::Settings>("AssetSetting::Settings");
    readSettings();
}

//==============================================================================

void WalletAssetsModel::addAsset(CoinAsset asset)
{
    _assets.emplace(asset.coinID(), asset);
    auto assetSettings = new AssetSettings(asset.coinID(), asset.misc().isAlwaysActive, true,
        AutopilotDetails(0.5, 1), asset.lndData().defaultSatsPerByte, this);

    connect(assetSettings, &AssetSettings::activeChanged, this, &WalletAssetsModel::writeSettings);
    connect(assetSettings, &AssetSettings::autopilotActiveChanged, this,
        &WalletAssetsModel::writeSettings);
    connect(
        assetSettings, &AssetSettings::satsPerByteChanged, this, &WalletAssetsModel::writeSettings);
    connect(
        assetSettings, &AssetSettings::backupDirChanged, this, &WalletAssetsModel::writeSettings);
    connect(assetSettings, &AssetSettings::autopilotDetailsChanged, this,
        &WalletAssetsModel::writeSettings);
    _activeAssets.emplace(asset.coinID(), assetSettings);
}

//==============================================================================

AssetSettings* WalletAssetsModel::assetSettings(AssetID assetID) const
{
    Q_ASSERT(_activeAssets.count(assetID) > 0);
    return _activeAssets.at(assetID);
}

//==============================================================================

const std::vector<AssetID> WalletAssetsModel::accountAssets() const
{
    std::vector<AssetID> result;
    for (auto&& assetID : activeAssets()) {
        if (assetById(assetID).type() == CoinAsset::Type::Account) {
            result.push_back(assetID);
        }
    }
    return result;
}

//==============================================================================

void WalletAssetsModel::writeSettings() const
{
    QSettings settings;
    QList<QVariant> settingsList;
    for (auto&& id : _activeAssets) {
        settingsList << id.second->save();
    }
    settings.setValue(SETTINGS_ACTIVE_ASSETS, settingsList);
    settings.sync();
}

//==============================================================================

void WalletAssetsModel::readSettings()
{
    QSettings settings;
    for (auto&& var : settings.value(SETTINGS_ACTIVE_ASSETS).toList()) {
        if (var.isValid()) {
            auto setting = var.value<AssetSettings::Settings>();
            if (_activeAssets.count(setting.assetID) > 0) {
                _activeAssets.at(setting.assetID)->load(setting);
                if (assetById(setting.assetID).misc().isAlwaysActive) {
                    _activeAssets.at(setting.assetID)->setIsActive(true);
                }
            }
        }
    }
}

//==============================================================================
