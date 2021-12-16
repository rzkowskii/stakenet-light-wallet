#include "ChannelRentingManager.hpp"
#include <Orderbook/OrderbookClient.hpp>
#include <Service/RefundableFeeManager.hpp>
#include <Swaps/AbstractSwapClient.hpp>
#include <Swaps/AbstractSwapClientPool.hpp>
#include <Swaps/AbstractSwapLndClient.hpp>
#include <Utils/GenericProtoDatabase.hpp>
#include <Utils/Logging.hpp>
#include <utilstrencodings.h>

#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <mutex>
#include <tinyformat.h>

using namespace boost::adaptors;

namespace orderbook {

static const std::string OPENING_STATE{ "OPENING" };

static const std::string CLOSED_STATE{ "CLOSED" };

//==============================================================================

template <class Container>
ChannelRentingManager::RentedChannels ChanellsToVector(const Container& container)
{
    ChannelRentingManager::RentedChannels channels;
    auto toVec = [](const auto& it) { return it.second; };
    boost::copy(container | transformed(toVec), std::back_inserter(channels));
    return channels;
}

//==============================================================================

ChannelRentingManager::ChannelRentingManager(const swaps::AbstractLndSwapClientPool& clients,
    swaps::RefundableFeeManager& feeManager, OrderbookClient* orderbook,
    std::shared_ptr<Utils::LevelDBSharedDatabase> sharedDb, QObject* parent)
    : QObject(parent)
    , _swapClients(clients)
    , _feeManager(feeManager)
    , _repository(std::make_unique<Repository>(sharedDb, "rented_channels"))
    , _orderbook(orderbook)
    , _renting(orderbook->renting())
{
    init();
}

//==============================================================================

ChannelRentingManager::~ChannelRentingManager() {}

//==============================================================================

Promise<RentingFee> ChannelRentingManager::feeToRentChannel(std::string requestedCurrency,
    std::string payingCurrency, int64_t capacity, int64_t lifetimeSeconds)
{
    return Promise<RentingFee>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto renting = _orderbook->renting();
            renting->feeToRentChannel(requestedCurrency, payingCurrency, capacity, lifetimeSeconds)
                .then([resolve, this](OrderbookChannelRenting::RentingFeeResponse feeInfo) {
                    resolve(RentingFee(ConvertBigInt(feeInfo.fee()),
                        feeInfo.has_rentingfee() ? ConvertBigInt(feeInfo.rentingfee()) : 0,
                        feeInfo.has_onchainfees() ? ConvertBigInt(feeInfo.onchainfees()) : 0));
                })
                .fail(
                    [reject]() { reject(std::runtime_error("Failed to get rental channel fee")); });
        });
    });
}

//==============================================================================

Promise<OrderbookChannelRenting::RentResponse> ChannelRentingManager::connextRent(
    std::string requestedCurrency, std::string payingCurrency, int64_t rentedAmount,
    int64_t lifetimeSeconds)
{
    return Promise<OrderbookChannelRenting::RentResponse>(
        [=](const auto& resolve, const auto& reject) {
            QMetaObject::invokeMethod(this, [=] {
                _renting
                    ->generatePaymentHashToRentChannel(
                        requestedCurrency, payingCurrency, rentedAmount, lifetimeSeconds)
                    .then([this, requestedCurrency, payingCurrency, rentedAmount](
                              OrderbookChannelRenting::PaymentHash paymentHash) {
                        return _feeManager.payChannelRentingFee(
                            requestedCurrency, payingCurrency, paymentHash, rentedAmount);
                    })
                    .then([this, payingCurrency](
                              swaps::RefundableFeeManager::RentingPaymentFeeResponse rentingInfo) {
                        return _renting
                            ->rent(QString::fromStdString(rentingInfo.paymentHash)
                                       .replace("0x", "")
                                       .toStdString(),
                                payingCurrency)
                            .tap([this, id = rentingInfo.fee.id()] {
                                _feeManager.burnRefundableFee({ id });
                            })
                            .tapFail([](const std::exception& ex) {
                                LogCCritical(Orderbook) << "Failed to rent channel: " << ex.what();
                            });
                    })
                    .then([resolve](OrderbookChannelRenting::RentResponse response) {
                        resolve(response);
                    })
                    .fail([reject](const std::exception& ex) {
                        LogCCritical(Swaps) << "Renting failure, reason: " << ex.what();
                        reject(std::current_exception());
                    });
            });
        });
}

//==============================================================================

Promise<OrderbookChannelRenting::RentResponse> ChannelRentingManager::lndRent(
    std::string requestedCurrency, std::string payingCurrency, int64_t rentedAmount,
    int64_t lifetimeSeconds)
{
    return Promise<OrderbookChannelRenting::RentResponse>(
        [=](const auto& resolve, const auto& reject) {
            QMetaObject::invokeMethod(this, [=] {
                _renting
                    ->generateRentInvoice(
                        requestedCurrency, payingCurrency, rentedAmount, lifetimeSeconds)
                    .then([this, requestedCurrency, payingCurrency, rentedAmount](
                              OrderbookChannelRenting::RentInvoice paymentRequest) {
                        return _feeManager.payChannelRentingFee(
                            requestedCurrency, payingCurrency, paymentRequest, rentedAmount);
                    })
                    .then([this, payingCurrency](
                              swaps::RefundableFeeManager::RentingPaymentFeeResponse rentingInfo) {
                        return _renting->rent(rentingInfo.paymentHash, payingCurrency)
                            .tap([this, id = rentingInfo.fee.id()] {
                                _feeManager.burnRefundableFee({ id });
                            })
                            .tapFail([](const std::exception& ex) {
                                LogCCritical(Orderbook) << "Failed to rent channel: " << ex.what();
                            });
                    })
                    .then([resolve](OrderbookChannelRenting::RentResponse response) {
                        resolve(response);
                    })
                    .fail([reject](const std::exception& ex) {
                        LogCCritical(Swaps) << "Renting failure, reason: " << ex.what();
                        reject(std::current_exception());
                    });
            });
        });
}

//==============================================================================

Promise<storage::RentedChannel> ChannelRentingManager::extendTime(
    std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds)
{
    return Promise<storage::RentedChannel>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            //            OrderbookChannelRenting::InvoiceToExtendRentedChannel paymentRequest;
            //            OrderbookChannelRenting::PaymentHashToExtendChannel paymentHash;
            std::string request;

            if (swaps::CLIENT_TYPE_PER_CURRENCY.at(payingCurrency) == swaps::ClientType::Lnd) {
                _renting
                    ->generateInvoiceToExtendRentedChannel(
                        channelId, payingCurrency, lifetimeSeconds)
                    .then([this, &request](
                              OrderbookChannelRenting::InvoiceToExtendRentedChannel payReq) {
                        request = payReq;
                    })
                    .fail([reject]() {
                        LogCCritical(Swaps) << "generateInvoiceToExtendRentedChannel failed";
                        reject(std::current_exception());
                    })
                    .wait();
            } else {
                _renting
                    ->generatePaymentHashToExtendConnextRentedChannel(
                        channelId, payingCurrency, lifetimeSeconds)
                    .then([this, &request](
                              OrderbookChannelRenting::PaymentHashToExtendChannel payHash) {
                        request = payHash;
                    })
                    .fail([reject]() {
                        LogCCritical(Swaps)
                            << "generatePaymentHashToExtendConnextRentedChannel failed";
                        reject(std::current_exception());
                    })
                    .wait();
                reject("Eth fees currently disabled on backend");
            }

            _feeManager.payExtendTimeRentingFee(payingCurrency, request)
                .then([this, payingCurrency](
                          swaps::RefundableFeeManager::ExtendPaymentFeeResponse response) {
                    return _renting->extendRentedChannelTime(response.paymentHash, payingCurrency)
                        .tap([this, id = response.fee.id()] {
                            _feeManager.burnRefundableFee({ id });
                        })
                        .tapFail([](const std::exception& ex) {
                            LogCCritical(Orderbook) << "Failed to extend channel: " << ex.what();
                            throw std::runtime_error(
                                "extending rental channel on remote side failed");
                        });
                })
                .then([this](OrderbookChannelRenting::ExtendRentedChannelTimeResponse response) {
                    auto channelId = response.channelid();
                    LogCDebug(Swaps) << "Succesfully extended channel:" << channelId.data();

                    auto rentingChannels = ChanellsToVector(_repository->values());
                    auto it = std::find_if(rentingChannels.begin(), rentingChannels.end(),
                        [channelId](const storage::RentedChannel& channel) {
                            return channel.channelid() == channelId;
                        });
                    if (it != rentingChannels.end()) {
                        it->set_expiresat(response.expiresat());
                        it->set_channelid(response.channelid());
                        _repository->update({ *it });
                        this->channelChanged({ *it });
                        return *it;
                    } else {
                        LogCDebug(Swaps)
                            << "Cannot found channel when extending rental, channel id "
                            << channelId.data();
                        return storage::RentedChannel();
                    }
                })
                .then([resolve](storage::RentedChannel channel) { resolve(channel); })
                .fail([reject](const std::exception& ex) {
                    LogCCritical(Swaps) << "Extend renting channel failure, reason: " << ex.what();
                    reject(std::current_exception());
                });
        });
    });
}

//==============================================================================

Promise<int64_t> ChannelRentingManager::feeToExtendRentalChannel(
    std::string channelId, std::string payingCurrency, int64_t lifetimeSeconds)
{
    return Promise<int64_t>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            auto renting = _orderbook->renting();
            renting->feeToExtendedRentChannel(channelId, payingCurrency, lifetimeSeconds)
                .then([resolve](int64_t fee) { resolve(fee); })
                .fail([reject](const std::exception& ex) { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

Promise<storage::RentedChannel> ChannelRentingManager::rent(bool isLndType,
    std::string requestedCurrency, std::string payingCurrency, int64_t rentedAmount,
    int64_t lifetimeSeconds)
{
    return Promise<storage::RentedChannel>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(this, [=] {
            this->rentChannel(
                    isLndType, requestedCurrency, payingCurrency, rentedAmount, lifetimeSeconds)
                .then([this, isLndType, rentedAmount, requestedCurrency](
                          OrderbookChannelRenting::RentResponse response) {
                    storage::RentedChannel channel;
                    channel.set_capacity(rentedAmount);
                    channel.set_currency(requestedCurrency);
                    channel.set_rentingdate(QDateTime::currentMSecsSinceEpoch());

                    auto responceChannelType = response.channel().has_lndchannel()
                        ? storage::ChannelType::LND
                        : storage::ChannelType::CONNEXT;

                    channel.set_type(responceChannelType);
                    if (responceChannelType == storage::ChannelType::LND) {
                        channel.set_channelid(response.channel().lndchannel().channelid());
                        auto& lndDetails = *channel.mutable_lnddetails();
                        lndDetails.set_fundingoutpoint(
                            strprintf("%s:%lu", response.fundingtxidstr(), response.outputindex()));
                    } else {
                        channel.set_channelid(response.channel().connextchannel().channelid());
                        auto& connextDetails = *channel.mutable_connextdetails();
                        connextDetails.set_channeladdress(
                            response.channel().connextchannel().channeladdress());
                    }

                    LogCDebug(Swaps)
                        << "Succesfully rented for" << QString::fromStdString(requestedCurrency)
                        << "channel:" << channel.channelid().data();

                    _repository->save(channel);
                    this->channelAdded({ channel });
                    return channel;
                })
                .then([this, isLndType](storage::RentedChannel channel) {
                    LogCDebug(Swaps) << "Channel updated: " << channel.channelid().data();
                    return _renting->getRentChannelStatus(channel.channelid())
                        .then([this, channel](OrderbookChannelRenting::RentChannelStatusResponse
                                      response) mutable {
                            channel.set_expiresat(channel.type() == storage::ChannelType::LND
                                    ? response.lnd().expiresat()
                                    : response.connext().expiresat());
                            channel.set_channelstatus(channel.type() == storage::ChannelType::LND
                                    ? response.lnd().status()
                                    : response.connext().status());
                            _repository->update({ channel });
                            this->channelChanged({ channel });
                            return channel;
                        });
                })
                .then([resolve](storage::RentedChannel channel) { resolve(channel); })
                .fail([reject](const std::exception& ex) {
                    LogCCritical(Swaps) << "Renting failure, reason: " << ex.what();
                    reject(std::current_exception());
                });
        });
    });
}

//==============================================================================

Promise<ChannelRentingManager::RentedChannels> ChannelRentingManager::channels()
{
    return Promise<RentedChannels>([this](const auto& resolve, const auto& /*reject*/) {
        QMetaObject::invokeMethod(this, [=] { resolve(ChanellsToVector(_repository->values())); });
    });
}

//==============================================================================

void ChannelRentingManager::refresh()
{
    QMetaObject::invokeMethod(
        this, [this] { this->updateChannelsStatus(ChanellsToVector(_repository->values())); });
}

//==============================================================================

void ChannelRentingManager::onUpdateStateTimeout()
{
    if (_orderbook->state() != OrderbookApiClient::State::Connected) {
        LogCWarning(Orderbook) << "Skipping channel renting manager scheduled update";
        return;
    }
    RentedChannels updates;
    auto transform = [](const auto& it) { return it.second; };
    auto currentTs = QDateTime::currentSecsSinceEpoch();
    auto filter = [currentTs](const std::pair<uint64_t, storage::RentedChannel>& it) {
        return (it.second.channelstatus().empty() || it.second.channelstatus() == OPENING_STATE)
            || it.second.expiresat() > currentTs;
    };

    boost::copy(_repository->values() | filtered(filter) | transformed(transform),
        std::back_inserter(updates));
    updateChannelsStatus(updates);
}

//==============================================================================

void ChannelRentingManager::updateChannelsStatus(ChannelRentingManager::RentedChannels channels)
{
    using Status = OrderbookChannelRenting::RentChannelStatusResponse;
    QtPromise::map(channels,
        [this](storage::RentedChannel channel, ...) {
            return _renting->getRentChannelStatus(channel.channelid())
                .then([channel](Status status) { return std::make_pair(status, channel); })
                .fail([channel] {
                    LogCCritical(Orderbook) << "Get rent channel status failed!"
                                            << "channel id: " << channel.channelid().c_str();
                    return std::make_pair(Status(), channel);
                });
        })
        .then([this](QVector<std::pair<Status, storage::RentedChannel>> responses) {
            RentedChannels updates;
            auto transform = [](const std::pair<Status, storage::RentedChannel>& entry) {
                auto r = entry.second;
                auto type = r.type();
                r.set_channelstatus(type == storage::ChannelType::LND
                        ? entry.first.lnd().status()
                        : entry.first.connext().status());
                r.set_expiresat(type == storage::ChannelType::LND
                        ? entry.first.lnd().expiresat()
                        : entry.first.connext().expiresat());
                return r;
            };
            auto filter = [](const std::pair<Status, storage::RentedChannel>& entry) {
                return !entry.first.channelid().empty();
            };
            boost::copy(
                responses | filtered(filter) | transformed(transform), std::back_inserter(updates));
            _repository->update(updates);

            std::vector<uint64_t> closedChannelsId;
            auto transformCloseChannels
                = [](const std::pair<Status, storage::RentedChannel>& entry) {
                      return entry.second.id();
                  };
            auto closeFilter = [](const std::pair<Status, storage::RentedChannel>& entry) {
                return (entry.second.type() == storage::ChannelType::LND
                           && entry.first.lnd().status() == CLOSED_STATE)
                    || (entry.second.type() == storage::ChannelType::CONNEXT
                           && entry.first.connext().status() == CLOSED_STATE);
            };
            boost::copy(responses | filtered(closeFilter) | transformed(transformCloseChannels),
                std::back_inserter(closedChannelsId));
            _repository->erase(closedChannelsId);
            this->channelChanged(updates);
        });
}

//==============================================================================

Promise<OrderbookChannelRenting::RentResponse> ChannelRentingManager::rentChannel(bool isLndType,
    std::string requestedCurrency, std::string payingCurrency, int64_t rentedAmount,
    int64_t lifetimeSeconds)
{
    if (isLndType) {
        return lndRent(requestedCurrency, payingCurrency, rentedAmount, lifetimeSeconds);
    } else {
        return connextRent(requestedCurrency, payingCurrency, rentedAmount, lifetimeSeconds);
    }
}

//==============================================================================

void ChannelRentingManager::init()
{
    static std::once_flag once;
    std::call_once(once, [] { qRegisterMetaType<RentedChannels>("RentedChannels"); });

    _repository->load();
    auto onOrderbookStateChanged = [this](OrderbookApiClient::State state) {
        if (state == OrderbookApiClient::State::Connected) {
            this->refresh();
        }
    };
    connect(_orderbook, &OrderbookClient::stateChanged, this, onOrderbookStateChanged);
    onOrderbookStateChanged(_orderbook->state());

    auto updateTimer = new QTimer(this);
    connect(updateTimer, &QTimer::timeout, this, &ChannelRentingManager::onUpdateStateTimeout);
    updateTimer->setSingleShot(false);
    updateTimer->setInterval(60 * 1000);
    updateTimer->start();
}

//==============================================================================
}
