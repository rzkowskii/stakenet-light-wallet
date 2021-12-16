#ifndef WALLETSWAPVIEWMODEL_HPP
#define WALLETSWAPVIEWMODEL_HPP

#include <QObject>
#include <QPointer>
#include <Tools/Common.hpp>

class ApplicationViewModel;
class WalletMarketSwapModel;
class WalletAssetsModel;
struct MarketSwapRequest;

class WalletMarketSwapViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool marketSwapInProcess READ marketSwapInProcess NOTIFY marketSwapInProcessChanged)
    Q_PROPERTY(bool sendChannelState READ sendChannelState NOTIFY marketSwapInProcessChanged)
    Q_PROPERTY(bool recvChannelState READ recvChannelState NOTIFY marketSwapInProcessChanged)

public:
    explicit WalletMarketSwapViewModel(QObject* parent = nullptr);
    ~WalletMarketSwapViewModel();

    bool marketSwapInProcess() const;
    bool sendChannelState() const;
    bool recvChannelState() const;

public slots:
    void initialize(ApplicationViewModel* appViewModel);
    void executeSwap(MarketSwapRequest request, double feeSatsPerByte);

    void calculatePreSwapAmounts(AssetID sendAssetID, AssetID receiveAssetID,
                                 double sendAmount, double receiveAmount,
                                 double sendAssetFeeRate, double receiveAssetFeeRate, bool needToRentChannel);

    void evaluateChannels(AssetID sendAssetID, AssetID receiveAssetID, double sendAmount,
        double receiveAmount, bool isBuy, double placeOrderFee, double sendResv, double receiveResv);

signals:
    void marketSwapInProcessChanged();

    void openingCanSendChannelsFinished();
    void openingCanSendChannelsFailed(QString error);

    void openingCanReceiveChannelsFinished();
    void openingCanReceiveChannelsFailed(QString error);

    void evaluateChannelsRequested(MarketSwapRequest request);
    void evaluateChannelsFailed(QString error);

    void checkRentingFeeFailed(QString error);
    void checkChannelOnChainFeeFailed(QString error);

    void swapSuccess(double amount, QString receiveSymbol);
    void swapFailed(QString error);

    void rentalFeeRequested(double totalRentalFee);
    void rentalFeeFailed(QString error);

    void channelsReserveRequested(double sendAssetReserve, double receiveAssetReserve);

    void calculatePreSwapAmountsFinished(double totalRentalFee, double sendAssetReserve, double receiveAssetReserve);

private slots:
    void onCurrentSwapRequestChanged();
    void onSwapFailed(MarketSwapRequest request);

private:
    QPointer<WalletMarketSwapModel> _marketSwapModel;
    QPointer<WalletAssetsModel> _assetsModel;
};

#endif // WALLETSWAPVIEWMODEL_HPP
