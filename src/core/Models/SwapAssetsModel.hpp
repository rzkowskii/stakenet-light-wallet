#ifndef SwapAssetsModel_HPP
#define SwapAssetsModel_HPP

#include <QAbstractListModel>
#include <QObject>
#include <Tools/Common.hpp>

class WalletAssetsModel;
class DexService;
class DexStatusManager;

class SwapAssetsModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(std::vector<QString> baseAssets READ baseAssets NOTIFY tradingPairsChanged)
    Q_PROPERTY(std::vector<QString> quoteAssets READ quoteAssets NOTIFY tradingPairsChanged)
public:
    enum Roles {
        BaseAssetID,
        QuoteAssetID,
        BaseAssetSymbol,
        QuoteAssetSymbol,
        SwapPairFilterRole,
        DexPairStatusRole
    };

    explicit SwapAssetsModel(QObject* parent = nullptr);
    virtual ~SwapAssetsModel() override;

    virtual int rowCount(const QModelIndex& parent) const override final;
    virtual QVariant data(const QModelIndex& index, int role) const override final;
    virtual QHash<int, QByteArray> roleNames() const override final;

    std::vector<QString> baseAssets() const;
    std::vector<QString> quoteAssets() const;

signals:
    void dexStatusChanged();
    void tradingPairsChanged();

public slots:
    void initialize(QObject* appViewModel);
    QVariantMap get(int row);
    QVariantMap getByPair(QString swapPair);
    int lastTradingPairIndex();
    int lastBaseAssetID() const;
    int lastQuoteAssetID() const;
    bool hasPair(AssetID baseAssetID, AssetID quoteAssetID);

private slots:
    void onTradingPairsChanged();
    void onDexStatusChanged(AssetID assetID);

private:
    Enums::DexTradingStatus getDexStatus(AssetID baseID, AssetID quoteID) const;

private:
    std::vector<DexAssetPair> _impl;
    QPointer<WalletAssetsModel> _assetModel;
    QPointer<DexService> _dexService;
    QPointer<DexStatusManager> _dexStatusManager;
};

#endif // SwapAssetsModel_HPP
