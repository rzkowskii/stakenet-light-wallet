#ifndef WALLETMARKETSWAPMODEL_HPP
#define WALLETMARKETSWAPMODEL_HPP

#include <LndTools/LndTypes.hpp>
#include <LndTools/ConnextTypes.hpp>
#include <QObject>
#include <QPointer>
#include <QVariantMap>
#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <optional>

class LnDaemonsManager;
class WalletAssetsModel;
class DexService;
class PaymentNodesManager;

namespace orderbook {
class ChannelRentingManager;
struct RentingFee;
}

//==============================================================================

struct SwapFee {
    Q_GADGET
    Q_PROPERTY(double rentalFee MEMBER rentalFee)
    Q_PROPERTY(double onChainRentalFee MEMBER onChainRentalFee)
    Q_PROPERTY(double onChainChannelFee MEMBER onChainChannelFee)

public:
    Balance rentalFee{ 0 };
    Balance onChainRentalFee{ 0 }; // open + close fee from opening rental channel
    Balance onChainChannelFee{ 0 }; // on chain fee from opening can sent channel

    Balance placeOrderFee{ 0 };
    bool operator!=(const SwapFee& rhs) const
    {
        return onChainChannelFee != rhs.onChainChannelFee
            || onChainRentalFee != rhs.onChainRentalFee || rentalFee != rhs.rentalFee
            || placeOrderFee != rhs.placeOrderFee;
    }
};
Q_DECLARE_METATYPE(SwapFee)

//==============================================================================

struct MarketSwapRequest {
    Q_GADGET
    Q_PROPERTY(double sendAmount MEMBER sendAmount)
    Q_PROPERTY(double receiveAmount MEMBER receiveAmount)
    Q_PROPERTY(SwapFee fee MEMBER fee)

public:
    enum class State {
        Initial, // initial state when we create swap request
        Pending, // waiting for channels to be opened
        Executing, // posted order and swap in progress
        Completed, // swap succesfully completed
        Failure, // swap failed
        Unknown, // invalid state
    };
    Q_ENUM(State)

    AssetID sendAssetID;
    AssetID receiveAssetID;
    Balance sendAmount{ 0 };
    Balance receiveAmount{ 0 };
    SwapFee fee;
    bool isBuy{ false };
    Balance sendReserve{ 0 };
    Balance receiveReserve{ 0 };

    std::optional<Balance> missingSendBalance;
    std::optional<Balance> missingRecvBalance;
    std::optional<QString> rentedChannelId;
    std::optional<QString> openingChannelId;
    std::optional<bool> sendChannelOpened;
    std::optional<bool> recvChannelOpened;
    std::optional<QString> failureMsg;
    std::optional<uint32_t> retryCount;
    State state{ State::Initial };
};
Q_DECLARE_METATYPE(MarketSwapRequest)

//==============================================================================

class WalletMarketSwapModel : public QObject {
    Q_OBJECT

public:
    explicit WalletMarketSwapModel(const WalletAssetsModel& assetsModel,
        const DexService& dexService, const PaymentNodesManager& paymentNodesManager,
        QObject* parent = nullptr);

    void submitSwapRequest(MarketSwapRequest swapRequest);

    std::optional<MarketSwapRequest> currentSwapRequest() const;

    void executeSwap(MarketSwapRequest request, Balance feeSatsPerByte);
    Promise<MarketSwapRequest> evaluateChannels(AssetID sendAssetID, AssetID receiveAssetID,
        Balance sendAmount, Balance receiveAmount, bool isBuy, Balance placeOrderFee, Balance sendResv, Balance receiveResv);
    Promise<Balance> rentalFee(AssetID requestedAssetID, AssetID payingAssetID, Balance capacity);
    Promise<Balance> channelReserve(AssetID assetID, Balance amount, Balance feeRate);

signals:
    void swapExecuted(MarketSwapRequest request);
    void swapFailed(MarketSwapRequest request);
    void currentSwapRequestChanged();

private slots:
    void onDexServiceReady();
    void onSendActiveLndChannelsChanged(std::vector<LndChannel> activeChannels);
    void onRecvActiveLndChannelsChanged(std::vector<LndChannel> activeChannels);

    void onSendActiveConnextChannelsChanged(std::vector<ConnextChannel> activeChannels);
    void onRecvActiveConnextChannelsChanged(std::vector<ConnextChannel> activeChannels);

private:
    void processCurrentSwapRequest();
    void processRentalChannel();
    void processPendingState();
    void failCurrentRequest(QString errorMsg);

    Promise<QString> openCanSendChannel(
        AssetID sendAssetID, Balance sendAmount, Balance feeSatsPerByte);
    Promise<QString> openCanSendLndChannel(
        AssetID sendAssetID, Balance sendAmount, Balance feeSatsPerByte);
    Promise<QString> incleaseCanSendConnextChannel(
        AssetID sendAssetID, Balance sendAmount);
    Promise<QString> openCanReceiveChannel(
        AssetID receiveAssetID, AssetID paymentAssetID, Balance receiveAmount);
    Promise<orderbook::RentingFee> checkRentingFee(
        AssetID receiveAssetID, AssetID payingAssetID, Balance capacity);
    Promise<Balance> checkCanSendChannelFee(AssetID sendAssetID, Balance capacity);
    Promise<void> placeOrder(Balance sendAmount, Balance receiveAmount, bool isBuy);
    Promise<bool> hasBalanceOnChannels(AssetID assetID, Balance amount,
        bool canSendChannels); // true - canSendChannels, false - canReceiveChannels

private:
    const WalletAssetsModel& _assetsModel;
    QPointer<orderbook::ChannelRentingManager> _channelRentingManager;
    const DexService& _dexService;
    const PaymentNodesManager& _paymentNodesManager;
    std::optional<MarketSwapRequest> _currentSwapRequest;
    QObject* _executionContext{ nullptr };
};

//==============================================================================

#endif // WALLETMARKETSWAPMODEL_HPP
