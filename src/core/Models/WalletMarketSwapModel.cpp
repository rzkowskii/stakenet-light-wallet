#include "WalletMarketSwapModel.hpp"

#include <Data/WalletAssetsModel.hpp>
#include <Models/ConnextDaemonInterface.hpp>
#include <Models/ConnextDaemonsManager.hpp>
#include <Models/DexService.hpp>
#include <Models/LnDaemonInterface.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Models/WalletDexStateModel.hpp>
#include <Utils/Logging.hpp>

#include <LndTools/LndProcessManager.hpp>
#include <Service/ChannelRentingManager.hpp>

static const uint32_t SWAP_FAILURE_MAX_RETRY = 4;
static const uint32_t SWAP_FAILURE_RETRY_TIMEOUT = 10000;

//==============================================================================

WalletMarketSwapModel::WalletMarketSwapModel(const WalletAssetsModel& assetsModel,
    const DexService& dexService, const PaymentNodesManager& paymentNodesManager, QObject* parent)
    : QObject(parent)
    , _assetsModel(assetsModel)
    , _dexService(dexService)
    , _paymentNodesManager(paymentNodesManager)
    , _executionContext(new QObject(this))
{
    if (_dexService.isStarted()) {
        onDexServiceReady();
    } else {
        connect(
            &_dexService, &DexService::started, this, &WalletMarketSwapModel::onDexServiceReady);
    }
}

//==============================================================================

void WalletMarketSwapModel::submitSwapRequest(MarketSwapRequest swapRequest)
{
    if (_currentSwapRequest) {
        throw std::runtime_error("Can't submit request while another one is pending");
    }

    _currentSwapRequest = swapRequest;
    _currentSwapRequest->state = MarketSwapRequest::State::Initial;
    currentSwapRequestChanged();
}

//==============================================================================

std::optional<MarketSwapRequest> WalletMarketSwapModel::currentSwapRequest() const
{
    return _currentSwapRequest;
}

//==============================================================================

void WalletMarketSwapModel::executeSwap(MarketSwapRequest request, Balance feeSatsPerByte)
{
    submitSwapRequest(request);

    QPointer<WalletMarketSwapModel> self{ this };

    if (request.missingRecvBalance || request.missingSendBalance) {
        _currentSwapRequest->state = MarketSwapRequest::State::Pending;
        currentSwapRequestChanged();
    }

    if (request.missingSendBalance) {
        _currentSwapRequest->sendChannelOpened = false;
        openCanSendChannel(request.sendAssetID, *request.missingSendBalance, feeSatsPerByte)
            .then([self](QString channelIdentifier) {
                if (self && self->_currentSwapRequest) {
                    self->_currentSwapRequest->openingChannelId
                        = channelIdentifier; // channelIdentifier: is outpoint for lnd, is channel
                                             // address for connext
                    self->currentSwapRequestChanged();
                }
            })
            .fail([self](const std::exception& ex) {
                if (self) {
                    self->failCurrentRequest(QString::fromStdString(ex.what()));
                }
            });
    }

    if (request.missingRecvBalance) {
        processRentalChannel();
    }

    // means we have everything setup, not in pending state
    if (_currentSwapRequest->state == MarketSwapRequest::State::Initial) {
        processPendingState();
    }
}

//==============================================================================

Promise<MarketSwapRequest> WalletMarketSwapModel::evaluateChannels(AssetID sendAssetID,
    AssetID receiveAssetID, Balance sendAmount, Balance receiveAmount, bool isBuy,
    Balance placeOrderFee, Balance sendResv, Balance receiveResv)
{
    MarketSwapRequest request;
    request.sendAssetID = sendAssetID;
    request.receiveAssetID = receiveAssetID;
    request.sendAmount = sendAmount;
    request.receiveAmount = receiveAmount;
    request.isBuy = isBuy;
    request.fee.placeOrderFee = placeOrderFee;
    request.sendReserve = sendResv;
    request.receiveReserve = receiveResv;

    return hasBalanceOnChannels(request.receiveAssetID, receiveAmount, false)
        .then([this, request](bool hasBalance) mutable {
            auto prom = hasBalance ? QtPromise::resolve(request)
                                   : checkRentingFee(request.receiveAssetID, request.sendAssetID,
                                         request.receiveAmount)
                                         .then([request](orderbook::RentingFee feeInfo) mutable {
                                             request.fee.rentalFee = feeInfo.rentingFee;
                                             request.fee.onChainRentalFee = feeInfo.onChainFee;
                                             request.missingRecvBalance
                                                 = request.receiveAmount + request.receiveReserve;
                                             return request;
                                         });

            return prom.then([this](MarketSwapRequest request) {
                auto sendChannelBalance = request.sendAmount + request.fee.placeOrderFee
                    + request.fee.rentalFee + request.fee.onChainRentalFee;

                return hasBalanceOnChannels(request.sendAssetID, sendChannelBalance, true)
                    .then([this, sendChannelBalance, request](bool hasBalance) mutable {
                        if (!hasBalance) {
                            return checkCanSendChannelFee(request.sendAssetID, sendChannelBalance)
                                .then([request](int64_t fee) mutable {
                                    request.fee.onChainChannelFee = fee;
                                    request.missingSendBalance = request.sendAmount
                                        + request.sendReserve + request.fee.placeOrderFee
                                        + request.fee.rentalFee + request.fee.onChainRentalFee;
                                    return request;
                                });
                        }

                        return QtPromise::resolve(request);
                    });
            });
        });
}

//==============================================================================

Promise<Balance> WalletMarketSwapModel::rentalFee(
    AssetID requestedAssetID, AssetID payingAssetID, Balance capacity)
{
    return checkRentingFee(requestedAssetID, payingAssetID, capacity)
        .then([](orderbook::RentingFee feeInfo) mutable { return feeInfo.fee; });
}

//==============================================================================

Promise<Balance> WalletMarketSwapModel::channelReserve(AssetID assetID, Balance amount, Balance feeRate)
{
    return Promise<Balance>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto accountAssets = _assetsModel.accountAssets();
            auto it = std::find(accountAssets.begin(), accountAssets.end(), assetID);
            if (it != accountAssets.end()) {
                resolve(0);
            } else {
                resolve(BuildChannelReserve(amount, feeRate));
            }
        });
    });
}

//==============================================================================

void WalletMarketSwapModel::onDexServiceReady()
{
    _channelRentingManager = _dexService.channelRentingManager();
}

//==============================================================================

void WalletMarketSwapModel::onSendActiveLndChannelsChanged(
    std::vector<LndChannel> activeChannels) //
{
    if (!_currentSwapRequest) {
        return;
    }

    if (_currentSwapRequest->openingChannelId) {
        auto it = std::find_if(activeChannels.begin(), activeChannels.end(),
            [outpoint = *_currentSwapRequest->openingChannelId](
                const auto& channel) { return channel.channelOutpoint == outpoint; });

        if (it != std::end(activeChannels)) {
            _currentSwapRequest->sendChannelOpened = true;
            disconnect(sender(), nullptr, this, nullptr);
        }
    }

    processRentalChannel();
    processPendingState();
}

//==============================================================================

void WalletMarketSwapModel::onSendActiveConnextChannelsChanged(
    std::vector<ConnextChannel> activeChannels)
{
    if (!_currentSwapRequest) {
        return;
    }

    if (_currentSwapRequest->rentedChannelId) {
        auto it = std::find_if(activeChannels.begin(), activeChannels.end(),
            [channelAddress = *_currentSwapRequest->rentedChannelId](
                const auto& channel) { return channel.channelAddress == channelAddress; });

        if (it != std::end(activeChannels)) {
            _currentSwapRequest->recvChannelOpened = true;
            disconnect(sender(), nullptr, this, nullptr);
        }
    }

    processPendingState();
}

//==============================================================================

void WalletMarketSwapModel::onRecvActiveLndChannelsChanged(
    std::vector<LndChannel> activeChannels) //
{
    if (!_currentSwapRequest) {
        return;
    }

    if (_currentSwapRequest->rentedChannelId) {
        auto it = std::find_if(activeChannels.begin(), activeChannels.end(),
            [outpoint = *_currentSwapRequest->rentedChannelId](
                const auto& channel) { return channel.channelOutpoint == outpoint; });

        if (it != std::end(activeChannels)) {
            _currentSwapRequest->recvChannelOpened = true;
            disconnect(sender(), nullptr, this, nullptr);
        }
    }

    processPendingState();
}

//==============================================================================

void WalletMarketSwapModel::onRecvActiveConnextChannelsChanged(
    std::vector<ConnextChannel> activeChannels)
{
    if (!_currentSwapRequest) {
        return;
    }

    if (_currentSwapRequest->rentedChannelId) {
        auto it = std::find_if(activeChannels.begin(), activeChannels.end(),
            [channelAddress = *_currentSwapRequest->rentedChannelId](
                const auto& channel) { return channel.channelAddress == channelAddress; });

        if (it != std::end(activeChannels)) {
            _currentSwapRequest->recvChannelOpened = true;
            disconnect(sender(), nullptr, this, nullptr);
        }
    }

    processPendingState();
}

//==============================================================================

void WalletMarketSwapModel::processCurrentSwapRequest()
{
    if (!_currentSwapRequest) {
        LogCCritical(General) << "Trying to process swap request without active swap request";
        return;
    }
}

//==============================================================================

void WalletMarketSwapModel::processRentalChannel()
{
    if (!_currentSwapRequest || !_currentSwapRequest->missingRecvBalance) {
        return;
    }

    if (!_currentSwapRequest->recvChannelOpened) {
        if (_currentSwapRequest->sendChannelOpened.value_or(true)) {
            _currentSwapRequest->recvChannelOpened = false;
            openCanReceiveChannel(_currentSwapRequest->receiveAssetID,
                _currentSwapRequest->sendAssetID, *_currentSwapRequest->missingRecvBalance)
                .then([this](QString channelIdentifier) {
                    _currentSwapRequest->rentedChannelId = channelIdentifier;
                    currentSwapRequestChanged();
                })
                .fail([this](const std::exception& ex) {
                    failCurrentRequest(QString::fromStdString(ex.what()));
                });
        }
    }
}

//==============================================================================

void WalletMarketSwapModel::processPendingState()
{
    if (!_currentSwapRequest) {
        return;
    }

    if (_currentSwapRequest->sendChannelOpened.value_or(true)
        && _currentSwapRequest->recvChannelOpened.value_or(true)) {

        switch (_currentSwapRequest->state) {
        case MarketSwapRequest::State::Initial:
        case MarketSwapRequest::State::Pending:
        case MarketSwapRequest::State::Executing:
            break;
        default:
            return;
        }

        placeOrder(_currentSwapRequest->sendAmount, _currentSwapRequest->receiveAmount,
            _currentSwapRequest->isBuy)
            .then([this] {
                _currentSwapRequest->state = MarketSwapRequest::State::Completed;
                auto request = *_currentSwapRequest;
                this->swapExecuted(request);

                _currentSwapRequest.reset();
                currentSwapRequestChanged();
            })
            .fail([this](const std::exception& ex) {
                if (_currentSwapRequest->retryCount.value_or(0) >= SWAP_FAILURE_MAX_RETRY) {
                    this->failCurrentRequest(QString::fromStdString(ex.what()));
                } else {
                    if (_currentSwapRequest->retryCount) {
                        ++(*_currentSwapRequest->retryCount);
                    } else {
                        _currentSwapRequest->retryCount.emplace(0);
                    }
                    QTimer::singleShot(SWAP_FAILURE_RETRY_TIMEOUT, this,
                        &WalletMarketSwapModel::processPendingState);
                }
            });
    }
}

//==============================================================================

void WalletMarketSwapModel::failCurrentRequest(QString errorMsg)
{
    _currentSwapRequest->state = MarketSwapRequest::State::Failure;
    _currentSwapRequest->failureMsg = errorMsg;
    auto request = *_currentSwapRequest;
    this->swapFailed(request);

    _currentSwapRequest.reset();
    currentSwapRequestChanged();
}

//==============================================================================

Promise<QString> WalletMarketSwapModel::openCanSendChannel(
    AssetID sendAssetID, Balance sendAmount, Balance feeSatsPerByte)
{
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto nodeInterface = _paymentNodesManager.interfaceById(sendAssetID);

            if (nodeInterface->type() == Enums::PaymentNodeType::Lnd) {
                 openCanSendLndChannel(sendAssetID, sendAmount, feeSatsPerByte).then([resolve](QString outpoint) { resolve(outpoint); })
                .fail([reject] { reject(std::current_exception()); });
            } else {
                incleaseCanSendConnextChannel(sendAssetID, sendAmount).then([resolve](QString channelAddress){ resolve(channelAddress); })
                .fail([reject] { reject(std::current_exception()); });
            }
        });
    });
}
//==============================================================================

Promise<QString> WalletMarketSwapModel::openCanSendLndChannel(AssetID sendAssetID, Balance sendAmount, Balance feeSatsPerByte)
{
    auto lndInterface = qobject_cast<LnDaemonInterface*>(_paymentNodesManager.interfaceById(sendAssetID));
    connect(lndInterface, &LnDaemonInterface::channelsChanged, this,
        &WalletMarketSwapModel::onSendActiveLndChannelsChanged, Qt::UniqueConnection);

    auto hostList = lndInterface->processManager()->getNodeConf();
    if (!hostList.isEmpty()) {
        return lndInterface
            ->addChannelRequest(
                hostList.first(), QString::number(sendAmount), feeSatsPerByte);
    } else {
        return Promise<QString>::reject(std::runtime_error("No available public key for opening channel"));
    }
}

//==============================================================================

Promise<QString> WalletMarketSwapModel::incleaseCanSendConnextChannel(AssetID sendAssetID, Balance sendAmount)
{
    auto connextInterface = qobject_cast<ConnextDaemonInterface*>(_paymentNodesManager.interfaceById(sendAssetID));

    connect(connextInterface, &ConnextDaemonInterface::channelsChanged, this,
        &WalletMarketSwapModel::onSendActiveConnextChannelsChanged,
        Qt::UniqueConnection);

    return connextInterface->identifier()
        .then([connextInterface, sendAmount, sendAssetID](
                  QString identifier) {
            if (!identifier.isEmpty()) {
                return connextInterface->channels().then(
                    [connextInterface, sendAmount, sendAssetID, identifier](std::vector<ConnextChannel> channels) {
                        if (channels.empty()) {
                            return connextInterface
                                ->openChannel(identifier, sendAmount, sendAssetID);
                        } else {
                            return connextInterface
                                ->depositChannel(sendAmount, channels[0].channelAddress, sendAssetID);
                        }
                    });
            } else {
                return Promise<QString>::reject(
                    std::runtime_error("No available identifier for opening channel"));
            }
        })
        .fail([] { return Promise<QString>::reject(std::current_exception()); });
}

//==============================================================================

Promise<QString> WalletMarketSwapModel::openCanReceiveChannel(
    AssetID receiveAssetID, AssetID paymentAssetID, Balance receiveAmount)
{
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto currencySymbol = _assetsModel.assetById(receiveAssetID).ticket().toStdString();
            auto paymentSymbol = _assetsModel.assetById(paymentAssetID).ticket().toStdString();
            int64_t lifetimeSeconds = 1 * 60 * 60 * 24; // 1 day

            auto nodeInterface = _paymentNodesManager.interfaceById(receiveAssetID);

            auto isLnd = nodeInterface->type() == Enums::PaymentNodeType::Lnd;
            if (isLnd) {
                auto lndInterface = qobject_cast<LnDaemonInterface*>(nodeInterface);

                connect(lndInterface, &LnDaemonInterface::channelsChanged, this,
                    &WalletMarketSwapModel::onRecvActiveLndChannelsChanged, Qt::UniqueConnection);
            } else {
                auto connextInterface = qobject_cast<ConnextDaemonInterface*>(nodeInterface);

                connect(connextInterface, &ConnextDaemonInterface::channelsChanged, this,
                    &WalletMarketSwapModel::onRecvActiveConnextChannelsChanged,
                    Qt::UniqueConnection);
            }

            QPointer<WalletMarketSwapModel> self(this);
            _channelRentingManager
                ->rent(isLnd, currencySymbol, paymentSymbol, receiveAmount, lifetimeSeconds)
                .then([self, resolve](storage::RentedChannel channel) {
                    if (self) {
                        resolve(channel.type() == storage::ChannelType::LND
                                ? QString::fromStdString(channel.lnddetails().fundingoutpoint())
                                : QString::fromStdString(
                                      channel.connextdetails().channeladdress()));
                    }
                })
                .fail([self, reject](const std::exception&) { reject(std::current_exception()); })
                .fail([self, reject] {
                    reject(std::runtime_error("Failed to rent channel with unknown reason"));
                });
        });
    });
}

//==============================================================================

Promise<orderbook::RentingFee> WalletMarketSwapModel::checkRentingFee(
    AssetID receiveAssetID, AssetID payingAssetID, Balance capacity)
{
    return Promise<orderbook::RentingFee>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto currencySymbol = _assetsModel.assetById(receiveAssetID).ticket().toStdString();
            auto payingSymbol = _assetsModel.assetById(payingAssetID).ticket().toStdString();
            int64_t lifetimeSeconds = 3 * 60 * 60 * 24; // 1 day // 3 days

            _channelRentingManager
                ->feeToRentChannel(currencySymbol, payingSymbol, capacity, lifetimeSeconds)
                .then([resolve](orderbook::RentingFee feeInfo) { resolve(feeInfo); })
                .fail([reject] { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

Promise<Balance> WalletMarketSwapModel::checkCanSendChannelFee(
    AssetID sendAssetID, Balance capacity)
{
    return Promise<int64_t>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(0); });
    });
}

//==============================================================================

Promise<void> WalletMarketSwapModel::placeOrder(
    Balance sendAmount, Balance receiveAmount, bool isBuy)
{
    _currentSwapRequest->state = MarketSwapRequest::State::Executing;
    currentSwapRequestChanged();

    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            // pair XSN/LTC, base = XSN, quote = LTC
            // XSN <<< LTC = sell XSN, buy - LTC, sendAmount in XSN, receiveAmount in LTC
            // LTC <<< XSN = sell LTC, buy - XSN, sendAmount in LTC, receiveAmount in XSN
            // 1st arg is always in base currency
            // 2nd arg is always in quote currency
            _dexService.stateModel()
                .createOrder(isBuy ? receiveAmount : sendAmount, isBuy ? sendAmount : receiveAmount,
                    0, isBuy)
                .then([resolve, this] { resolve(); })
                .fail([reject, this] { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

Promise<bool> WalletMarketSwapModel::hasBalanceOnChannels(
    AssetID assetID, Balance amount, bool canSendChannels)
{
    return Promise<bool>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto nodeInterface = _paymentNodesManager.interfaceById(assetID);

            if (nodeInterface->type() == Enums::PaymentNodeType::Lnd) {
                auto lndInterface = qobject_cast<LnDaemonInterface*>(nodeInterface);

                if (!lndInterface) {
                    reject("Failed to get can send channels balance");
                }

                lndInterface->channels()
                    .then([](std::vector<LndChannel> channels) {
                        return BuildChannelsBalance(channels);
                    })
                    .then([this, resolve, amount, canSendChannels](
                              std::array<int64_t, 2> balancePair) {
                        resolve((canSendChannels ? balancePair[0] : balancePair[1]) >= amount);
                    });
            } else {
                auto connextInterface = qobject_cast<ConnextDaemonInterface*>(nodeInterface);

                if (!connextInterface) {
                    reject("Failed to get can send channels balance");
                }

                connextInterface->channels()
                    .then([assetID](std::vector<ConnextChannel> channels) {
                        auto weiBalances = BuildConnextChannelsBalance(channels);

                        std::array<int64_t, 2> result;
                        for (auto i = 0; i < weiBalances.size(); i++) {
                            result[i] = eth::ConvertFromWeiToSats(
                                weiBalances[i], UNITS_PER_CURRENCY.at(assetID), 8);
                        }
                        return result;
                    })
                    .then([this, resolve, amount, canSendChannels](
                              std::array<int64_t, 2> balancePair) {
                        resolve((canSendChannels ? balancePair[0] : balancePair[1]) >= amount);
                    });
            }
        });
    });
}

//==============================================================================
