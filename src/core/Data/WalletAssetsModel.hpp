#ifndef WALLETASSETSMODEL_HPP
#define WALLETASSETSMODEL_HPP

#include <Data/AssetSettings.hpp>
#include <QObject>

class WalletAssetsModel : public QObject {
    Q_OBJECT
public:
    using Assets = std::vector<CoinAsset>;
    explicit WalletAssetsModel(QString assetFilePath, QObject* parent = nullptr);
    virtual ~WalletAssetsModel();

    Assets assets() const;
    bool hasAsset(AssetID id) const;
    bool hasAsset(QString name) const;
    const CoinAsset& assetById(unsigned id) const;
    const CoinAsset& assetByName(QString name) const;
    const CoinAsset& assetByContract(QString contract) const;
    const std::vector<AssetID> activeAssets() const;
    AssetSettings* assetSettings(AssetID assetID) const;
    const std::vector<AssetID> accountAssets() const;

private:
    void init(QString assetFilePath);
    void addAsset(CoinAsset asset);
    void enableAsset(AssetID assetID);
    void disableAsset(AssetID assetID);
    void writeSettings() const;
    void readSettings();

private:
    std::map<AssetID, AssetSettings*> _activeAssets;
    std::map<AssetID, CoinAsset> _assets;
};

#endif // WALLETASSETSMODEL_HPP
