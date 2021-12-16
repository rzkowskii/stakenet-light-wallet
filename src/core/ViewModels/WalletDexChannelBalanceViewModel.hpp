#ifndef WALLETDEXCHANNELBALANCEVIEWMODEL_HPP
#define WALLETDEXCHANNELBALANCEVIEWMODEL_HPP

#include <QObject>
#include <Tools/Common.hpp>
#include <array>
#include <EthCore/Types.hpp>

class ApplicationViewModel;
class WalletAssetsModel;
class PaymentNodesManager;

class WalletDexChannelBalanceViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(AssetID baseAssetID READ baseAssetID NOTIFY tradingPairChanged)
    Q_PROPERTY(AssetID quoteAssetID READ quoteAssetID NOTIFY tradingPairChanged)
    Q_PROPERTY(QVariantMap channelsBalance READ channelsBalance NOTIFY channelsBalanceChanged)

public:
    explicit WalletDexChannelBalanceViewModel(QObject* parent = nullptr);

    AssetID baseAssetID() const;
    AssetID quoteAssetID() const;
    QVariantMap channelsBalance() const;

signals:
    void tradingPairChanged();
    void channelsBalanceChanged();

public slots:
    void initialize(ApplicationViewModel* applicationViewModel);
    void changeTraidingPair(AssetID baseAssetID, AssetID quoteAssetID);
    void reset();
    void initNodes();
    void initLndNode(AssetID assetID);
    void initConnextNode(AssetID assetID);

private:
    std::array<int64_t, 2> convertConnextBalancesToSats(const std::array<eth::u256, 2> weiBalanses, AssetID assetID);

private:
    QPointer<PaymentNodesManager> _paymentNodesManager;
    QPointer<WalletAssetsModel> _assetsModel;
    AssetID _baseAssetID;
    AssetID _quoteAssetID;
    using Balances = std::vector<Balance>;
    // std::array<int64_t, 2> { LocalBalances, RemoteBalances }
    std::array<int64_t, 2> _baseChannelsBalance;
    std::array<int64_t, 2> _quoteChannelsBalance;
};

#endif // WALLETDEXCHANNELBALANCEVIEWMODEL_HPP
