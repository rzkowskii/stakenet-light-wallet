#ifndef CHANNELRENTINGMANAGER_HPP
#define CHANNELRENTINGMANAGER_HPP

#include <Orderbook/OrderbookChannelRenting.hpp>
#include <Swaps/Protos/Storage.pb.h>
#include <Utils/Utils.hpp>

#include <QObject>
#include <unordered_map>

namespace Utils {
class LevelDBSharedDatabase;
template <class T> class GenericProtoDatabase;
}

namespace swaps {
class AbstractLndSwapClientPool;
class RefundableFeeManager;
}

namespace orderbook {

class OrderbookClient;

struct RentingFee {
    int64_t fee{ 0 }; // fee = rentingFee + onChainFees
    int64_t rentingFee{ 0 };
    int64_t onChainFee{ 0 };

    RentingFee() = default;
    RentingFee(int64_t fee, int64_t rentingFee, int64_t onChainFee)
        : fee(fee)
        , rentingFee(rentingFee)
        , onChainFee(onChainFee)
    {
    }
    bool operator!=(const RentingFee& rhs) const
    {
        return rentingFee != rhs.fee || onChainFee != rhs.onChainFee || fee != rhs.fee;
    }
};

class ChannelRentingManager : public QObject {
    Q_OBJECT
public:
    using RentedChannels = std::vector<storage::RentedChannel>;
    using ChannelType = storage::ChannelType;

    explicit ChannelRentingManager(const swaps::AbstractLndSwapClientPool& clients,
        swaps::RefundableFeeManager& feeManager, OrderbookClient* orderbook,
        std::shared_ptr<Utils::LevelDBSharedDatabase> sharedDb, QObject* parent = nullptr);
    ~ChannelRentingManager();

    Promise<RentingFee> feeToRentChannel(std::string requestedCurrency, std::string payingCurrency,
        int64_t capacity, int64_t lifetimeSeconds);

    Promise<storage::RentedChannel> rent(bool isLndType, std::string requestedCurrency,
        std::string payingCurrency, int64_t rentedAmount, int64_t lifetimeSeconds);

    Promise<storage::RentedChannel> extendTime(
        std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds);

    Promise<int64_t> feeToExtendRentalChannel(
        std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds);

    Promise<RentedChannels> channels();
    void refresh();

signals:
    void channelAdded(RentedChannels details);
    void channelChanged(RentedChannels details);

private slots:
    void onUpdateStateTimeout();

private:
    void updateChannelsStatus(RentedChannels channels);
    std::string channelIdByTxOutpoint(std::string fundingTxOutpoint) const;

    Promise<OrderbookChannelRenting::RentResponse> rentChannel(bool isLndType,
        std::string requestedCurrency, std::string payingCurrency, int64_t rentedAmount,
        int64_t lifetimeSeconds);

    Promise<OrderbookChannelRenting::RentResponse> lndRent(std::string requestedCurrency,
        std::string payingCurrency, int64_t rentedAmount, int64_t lifetimeSeconds);

    Promise<OrderbookChannelRenting::RentResponse> connextRent(std::string requestedCurrency,
        std::string payingCurrency, int64_t rentedAmount, int64_t lifetimeSeconds);

    void init();

private:
    using Repository = Utils::GenericProtoDatabase<storage::RentedChannel>;
    const swaps::AbstractLndSwapClientPool& _swapClients;
    swaps::RefundableFeeManager& _feeManager;
    std::unique_ptr<Repository> _repository;
    OrderbookClient* _orderbook{ nullptr };
    OrderbookChannelRenting* _renting{ nullptr };
};
}

#endif // CHANNELRENTINGMANAGER_HPP
