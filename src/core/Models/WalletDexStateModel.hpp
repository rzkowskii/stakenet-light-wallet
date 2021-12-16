#ifndef WALLETDEXSTATEMODEL_HPP
#define WALLETDEXSTATEMODEL_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <memory>
#include <optional>
#include <Swaps/Types.hpp>

class AbstractOrderbookDataSource;
class TradeHistoryListModel;
class ApplicationViewModel;
class OrderBookListModel;
class AskOrderBookModel;
class OwnOrdersDataSource;
class OwnOrdersListModel;
class WalletAssetsModel;
class OrderbookClient;
class DexService;
class OwnOrdersHistoryListModel;

//==============================================================================

QString SwapFailureMsg(swaps::SwapFailureReason swapReason);
QString ErrorMessageFromSwapFailure(const swaps::SwapFailure& failure);

//==============================================================================

class WalletDexStateModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(TradeHistoryListModel* tradeHistoryListModel READ tradeHistoryListModel NOTIFY
            swapAssetsChanged)
    Q_PROPERTY(
        OrderBookListModel* sellOrdersListModel READ sellOrdersListModel NOTIFY swapAssetsChanged)
    Q_PROPERTY(
        OrderBookListModel* buyOrdersListModel READ buyOrdersListModel NOTIFY swapAssetsChanged)
    Q_PROPERTY(OwnOrdersListModel* ownOrderBookListModel READ ownOrderBookListModel NOTIFY
            swapAssetsChanged)
    Q_PROPERTY(OwnOrdersHistoryListModel* ownOrdersHistoryListModel READ ownOrdersHistoryListModel
            NOTIFY swapAssetsChanged)
public:
    using SwapAssets = std::pair<AssetID, AssetID>;
    explicit WalletDexStateModel(
        const WalletAssetsModel& assetsModel, DexService& dexService, QObject* parent = nullptr);

    TradeHistoryListModel* tradeHistoryListModel();
    OrderBookListModel* sellOrdersListModel();
    OrderBookListModel* buyOrdersListModel();
    OwnOrdersListModel* ownOrderBookListModel();
    OwnOrdersHistoryListModel* ownOrdersHistoryListModel();

    bool hasSwapPair() const;
    QString getPairId() const;

    AssetID quoteAssetID() const;
    AssetID baseAssetID() const;

    Promise<void> createOrder(
        Balance baseQuantity, Balance quoteQuantity, double price, bool isBuy);

signals:
    void swapAssetsChanged();
    void noHubRouteError(AssetID assetID);

private slots:
    void setCurrentSwapAssets(std::optional<SwapAssets> swapAssets);

private:
    void updateCurrentModels();
    void writeSettings();

private:
    using TradeHistoryListModelPtr = std::unique_ptr<TradeHistoryListModel>;
    using OrderBookListModelPtr = std::unique_ptr<OrderBookListModel>;
    using OrderBookDataSourcePtr = std::unique_ptr<AbstractOrderbookDataSource>;
    using OwnOrdersHistoryListModelPtr = std::unique_ptr<OwnOrdersHistoryListModel>;
    std::map<SwapAssets, TradeHistoryListModelPtr> _tradeHistoryListModels;
    std::map<SwapAssets, OwnOrdersHistoryListModelPtr> _ownOrdersHistoryListModel;
    std::map<SwapAssets, OrderBookListModelPtr> _buyOrdersListModels;
    std::map<SwapAssets, OrderBookListModelPtr> _sellOrdersListModels;
    std::unique_ptr<OwnOrdersListModel> _ownOrdersModel;

    const WalletAssetsModel& _walletAssetsModel;
    DexService& _dexService;

    std::unique_ptr<OwnOrdersDataSource> _ownOrdersDataSource;
    std::map<SwapAssets, OrderBookDataSourcePtr> _sellOrdersDataSource;
    std::map<SwapAssets, OrderBookDataSourcePtr> _buyOrdersDataSource;

    std::optional<SwapAssets> _currentSwapAssets;
};

#endif // WALLETDEXSTATEMODEL_HPP
