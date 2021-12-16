#include <LndTools/LndGrpcClient.hpp>
#include <LndTools/LndProcessManager.hpp>
#include <Models/LnDaemonInterface.hpp>
#include <Tools/AppConfig.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QtDebug>
#include <cmath>
#include <functional>
#include <iostream>
#include <utilstrencodings.h>

using grpc::ClientAsyncReader;
using grpc::ClientAsyncResponseReader;
using grpc::ClientContext;
using grpc::CompletionQueue;
using grpc::Status;
using namespace lnrpc;

//==============================================================================

static Balance CalculateLocalBalance(std::vector<LndChannel> channels)
{
    return std::accumulate(std::begin(channels), std::end(channels), Balance(0),
        [](const auto& accum, const auto& channel) {
            return accum + std::max(channel.localBalance, Balance(0));
        });
}

//==============================================================================

static Balance CalculateLocalAndRemoteBalance(std::vector<LndChannel> channels)
{
    return std::accumulate(std::begin(channels), std::end(channels), Balance(0),
        [](const auto& accum, const auto& channel) {
            return accum + std::max(channel.localBalance, Balance(0))
                + std::max(channel.remoteBalance, Balance(0));
        });
}

//==============================================================================

static Balance CalculateAvailableLocalBalance(std::vector<LndChannel> channels)
{
    return std::accumulate(std::begin(channels), std::end(channels), Balance(0),
        [](const auto& accum, const auto& channel) {
            return accum
                + std::max(channel.localBalance - static_cast<Balance>(channel.capacity * 0.01)
                          - CHANNEL_RESERVE,
                      Balance(0));
        });
}

//==============================================================================

LnDaemonInterface::LnDaemonInterface(
    Cfg config, AssetLndData lndData, std::unique_ptr<LndGrpcClient> client, QObject* parent)
    : PaymentNodeInterface(Enums::PaymentNodeType::Lnd, parent)
    , _grpcClient(client.release())
    , _executionContext(new QObject(this))
{

    AssetLndConfig assetData{ lndData.confirmationForChannelApproved };

    _processManager.reset(new LndProcessManager(
        _grpcClient.get(), config.daemonConfig, config.rootDataDir, config.grpcPort, assetData));
    _channelsUpdateTimer = new QTimer(this);
    _autoConnectTimer = new QTimer(this);
    _miscUpdateTimer = new QTimer(this);

    _channelsUpdateTimer->setSingleShot(false);
    _autoConnectTimer->setSingleShot(false);
    _miscUpdateTimer->setSingleShot(false);

    _channelsUpdateTimer->setInterval(15 * 1000);
    _autoConnectTimer->setInterval(15 * 1000);
    _miscUpdateTimer->setInterval(30 * 1000);

    connect(_channelsUpdateTimer, &QTimer::timeout, this, &LnDaemonInterface::refreshChannels);
    connect(_autoConnectTimer, &QTimer::timeout, this, &LnDaemonInterface::tryConnectingToPeers);
    connect(_autoConnectTimer, &QTimer::timeout, this, &LnDaemonInterface::tryAddingWatchtowers);
    connect(_miscUpdateTimer, &QTimer::timeout, this, &LnDaemonInterface::doHouseKeeping);
    connect(_processManager.get(), &AbstractLndProcessManager::runningChanged, this,
        &LnDaemonInterface::onRunningChanged);
}

//==============================================================================

LnDaemonInterface::~LnDaemonInterface() {}

//==============================================================================

Promise<bool> LnDaemonInterface::isChannelsOpened() const
{
    return Promise<bool>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_channels.size() > 0); });
    });
}

//==============================================================================

Promise<std::vector<LndChannel>> LnDaemonInterface::channels() const
{
    return Promise<std::vector<LndChannel>>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_channels); });
    });
}

//==============================================================================

Promise<std::vector<LndChannel>> LnDaemonInterface::inactiveChannels() const
{
    return Promise<std::vector<LndChannel>>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_inactiveChannels); });
    });
}

//==============================================================================

Promise<std::vector<LndChannel>> LnDaemonInterface::pendingChannels() const
{
    return Promise<std::vector<LndChannel>>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_pendingOpenChannels); });
    });
}

//==============================================================================

Promise<std::vector<LndChannel>> LnDaemonInterface::pendingWaitingCloseChannels() const
{
    return Promise<std::vector<LndChannel>>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(
            _executionContext, [=] { resolve(_pendingWaitingCloseChannels); });
    });
}

//==============================================================================

Promise<std::vector<LndChannel>> LnDaemonInterface::pendingForceClosingChannels() const
{
    return Promise<std::vector<LndChannel>>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(
            _executionContext, [=] { resolve(_pendingForceClosingChannels); });
    });
}

//==============================================================================

Promise<LnDaemonInterface::LnBalance> LnDaemonInterface::balance() const
{
    return Promise<LnBalance>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_balance); });
    });
}

//==============================================================================

AbstractPaymentNodeProcessManager* LnDaemonInterface::processManager() const
{
    return _processManager.get();
}

//==============================================================================

Promise<QString> LnDaemonInterface::identifier() const
{
    return Promise<QString>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_infoPubKey); });
    });
}

//==============================================================================

Promise<int> LnDaemonInterface::peersNumber() const
{
    return Promise<int>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(this->peersNumberInternal()); });
    });
}

//==============================================================================

Promise<LnDaemonInterface::ChainSynced> LnDaemonInterface::syncedToChain() const
{
    return Promise<ChainSynced>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_chainSynced); });
    });
}

//==============================================================================

Promise<QString> LnDaemonInterface::openChannel(
    QString pubkey, int64_t localAmount, unsigned feeSatsPerByte)
{
    OpenChannelRequest req;
    auto bytes = bitcoin::ParseHex(pubkey.toStdString());
    req.set_node_pubkey(std::string(bytes.begin(), bytes.end()));
    req.set_local_funding_amount(localAmount);
    req.set_sat_per_byte(feeSatsPerByte);
    req.set_subtract_fees(true);
    // req.set_private_(true);

    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        auto context = ObserveAsync<OpenStatusUpdate>(this,
            [resolve](OpenStatusUpdate update) {
                auto txid = QByteArray::fromStdString(update.chan_pending().txid());
                std::reverse(std::begin(txid), std::end(txid));
                resolve(QString("%1:%2")
                            .arg(QString(txid.toHex()))
                            .arg(update.chan_pending().output_index()));
            },
            [reject](auto status) { reject(std::runtime_error(status.error_message())); });

        _grpcClient->makeRpcStreamingRequest(
            &Lightning::Stub::PrepareAsyncOpenChannel, req, std::move(context), 0);
    });
}

//==============================================================================

void LnDaemonInterface::fetchLndInfo()
{
    _grpcClient
        ->makeRpcUnaryRequest<GetInfoResponse>(
            &Lightning::Stub::PrepareAsyncGetInfo, GetInfoRequest())
        .then([this](GetInfoResponse response) {
            QString pubKey = QString::fromStdString(response.identity_pubkey());
            int numConnections = response.num_peers();

            _infoPubKey = pubKey;
            pubKeyChanged(_infoPubKey);

            _peersNum = numConnections;
            numberOfPeersChanged(_peersNum);

            _chainSynced = { response.synced_to_chain(), response.synced_to_graph() };
            chainSyncedChanged(_chainSynced.chain, _chainSynced.graph);
        })
        .fail([](Status status) {
            LogCDebug(Lnd) << "Failed to execute getinfo" << status.error_message().c_str();
            throw;
        });
}

//==============================================================================

void LnDaemonInterface::lndGetListChannels()
{
    lndGetActiveChannels();
    lndGetPendingChannels();
}

//==============================================================================

void LnDaemonInterface::onRunningChanged()
{
    if (_processManager->running()) {
        _grpcClient->connect();
        _miscUpdateTimer->start();
        _autoConnectTimer->start();
        _channelsUpdateTimer->start();

        QTimer::singleShot(4000, this, [this] {
            if (_grpcClient->isConnected()) {
                refreshChannels();
                doHouseKeeping();
            }
        });
    } else {
        _grpcClient->close();
        _miscUpdateTimer->stop();
        _autoConnectTimer->stop();
        _channelsUpdateTimer->stop();

        _balance = LnBalance();
        _infoPubKey = QString();
        _peersNum = 0;
        _chainSynced = {};
        _connectedToHub = false;

        _channels.clear();
        _inactiveChannels.clear();
        _pendingWaitingCloseChannels.clear();
        _pendingWaitingCloseChannels.clear();
        _pendingForceClosingChannels.clear();
    }
}

//==============================================================================

Promise<void> LnDaemonInterface::closeChannelByFundingTxid(
    QStringList channelPointInfo, bool isChannelInactive, unsigned feeSatsPerByte)
{
    auto channelPoint = new ChannelPoint;
    channelPoint->set_funding_txid_str(channelPointInfo[0].toStdString());
    channelPoint->set_output_index(std::stoi(channelPointInfo[1].toStdString()));

    CloseChannelRequest req;
    req.set_allocated_channel_point(channelPoint);
    req.set_force(isChannelInactive);
    req.set_sat_per_byte(feeSatsPerByte);

    return Promise<void>([=](const auto& resolve, const auto& reject) {
        auto context = ObserveAsync<CloseStatusUpdate>(this, [resolve](auto) { resolve(); },
            [reject](auto status) {
                LogCCritical(General)
                    << "Failed to execute closeChannel" << status.error_message().c_str();
                reject();
            });

        _grpcClient->makeRpcStreamingRequest(
            &Lightning::Stub::PrepareAsyncCloseChannel, req, std::move(context), 0);
    });
}

//==============================================================================

void LnDaemonInterface::lndGetActiveChannels()
{
    ListChannelsRequest req;
    req.set_active_only(true);
    req.set_inactive_only(true);
    _grpcClient
        ->makeRpcUnaryRequest<ListChannelsResponse>(
            &Lightning::Stub::PrepareAsyncListChannels, ListChannelsRequest())
        .then([this](ListChannelsResponse response) {
            std::vector<LndChannel> tempActive;
            std::vector<LndChannel> tempInactive;

            for (auto&& channel : response.channels()) {
                if (channel.active()) {
                    LndChannel activeChannel = LndChannel::FromRpcChannelLndChannel(channel);
                    activeChannel.details["local_chan_reserve_sat"]
                        = QVariant::fromValue(channel.local_chan_reserve_sat());
                    activeChannel.details["initiator"] = QVariant::fromValue(channel.initiator());
                    activeChannel.details["fee_per_kw"] = QVariant::fromValue(channel.fee_per_kw());
                    activeChannel.details["commit_weight"]
                        = QVariant::fromValue(channel.commit_weight());
                    tempActive.emplace_back(activeChannel);
                } else {
                    tempInactive.emplace_back(LndChannel::FromRpcChannelLndChannel(channel));
                }
            }

            _channels.swap(tempActive);
            _inactiveChannels.swap(tempInactive);
            channelsChanged(_channels, _inactiveChannels);

            auto newBalance = _balance;
            newBalance.active = CalculateLocalBalance(_channels);
            newBalance.inactive = CalculateLocalBalance(_inactiveChannels);

            newBalance.allLocal = BuildChannelsBalance(_channels).at(0);
            newBalance.allRemote = BuildChannelsBalance(_channels).at(1);

            newBalance.availableActive = CalculateAvailableLocalBalance(_channels);
            newBalance.availableInactive = CalculateAvailableLocalBalance(_inactiveChannels);

            newBalance.allActive = CalculateLocalAndRemoteBalance(_channels);
            newBalance.allInactive = CalculateLocalAndRemoteBalance(_inactiveChannels);

            if (newBalance != _balance) {
                _balance = newBalance;
                balanceChanged(_balance);
            }

            hasActiveChannelChanged(!_channels.empty());
        })
        .fail([](Status status) {
            LogCDebug(Lnd) << "Failed to execute AsyncListChannels"
                           << status.error_message().c_str();
            throw;
        });
}

//==============================================================================

void LnDaemonInterface::lndGetPendingChannels()
{
    _grpcClient
        ->makeRpcUnaryRequest<PendingChannelsResponse>(
            &Lightning::Stub::PrepareAsyncPendingChannels, PendingChannelsRequest())
        .then([this](PendingChannelsResponse response) {
            std::vector<LndChannel> tempPendingOpen;

            for (auto&& channel : response.pending_open_channels()) {
                tempPendingOpen.emplace_back(
                    LndChannel::FromRpcChannelPendingChannel(channel.channel()));
            }
            _pendingOpenChannels.swap(tempPendingOpen);
            pendingChannelsChanged(_pendingOpenChannels);
            hasPendingChannelChanged(!_pendingOpenChannels.empty());

            tempPendingOpen.clear();

            for (auto&& channel : response.waiting_close_channels()) {
                LndChannel waitingCloseChannel
                    = LndChannel::FromRpcChannelPendingChannel(channel.channel());
                waitingCloseChannel.details["closing_txid"]
                    = QString::fromStdString(channel.commitments().local_txid());
                tempPendingOpen.emplace_back(waitingCloseChannel);
            }

            _pendingWaitingCloseChannels.swap(tempPendingOpen);
            pendingWaitingCloseChannelsChanged(_pendingWaitingCloseChannels);

            tempPendingOpen.clear();

            for (auto&& channel : response.pending_force_closing_channels()) {
                LndChannel pendingForceClosingChannel
                    = LndChannel::FromRpcChannelPendingChannel(channel.channel());
                pendingForceClosingChannel.details["blocks_til_maturity"]
                    = channel.blocks_til_maturity();
                pendingForceClosingChannel.details["maturity_height"] = channel.maturity_height();
                pendingForceClosingChannel.details["closing_txid"]
                    = QString::fromStdString(channel.closing_txid());
                tempPendingOpen.emplace_back(pendingForceClosingChannel);
            }

            _pendingForceClosingChannels.swap(tempPendingOpen);
            pendingForceClosingChannelsChanged(_pendingForceClosingChannels);

            auto newBalance = _balance;
            newBalance.pending = CalculateLocalBalance(_pendingOpenChannels);
            newBalance.closing = CalculateLocalBalance(_pendingWaitingCloseChannels)
                + CalculateLocalBalance(_pendingForceClosingChannels);

            newBalance.availablePending = CalculateAvailableLocalBalance(_pendingOpenChannels);

            newBalance.allPending = CalculateLocalAndRemoteBalance(_pendingOpenChannels);
            newBalance.allClosing = CalculateLocalAndRemoteBalance(_pendingWaitingCloseChannels)
                + CalculateLocalAndRemoteBalance(_pendingForceClosingChannels);

            if (newBalance != _balance) {
                _balance = newBalance;
                balanceChanged(_balance);
            }
        })
        .fail([](Status status) {
            LogCDebug(Lnd) << "Failed to execute AsyncPendingChannels"
                           << status.error_message().c_str();
            throw;
        });
}

//==============================================================================

void LnDaemonInterface::setConnectedToHub(bool value)
{
    if (value != _connectedToHub) {
        _connectedToHub = value;
        numberOfPeersChanged(peersNumberInternal());
    }
}

//==============================================================================

void LnDaemonInterface::setTowersConnected(bool value)
{
    if (value != _towersConnected) {
        _towersConnected = value;
    }
}

//==============================================================================

int LnDaemonInterface::peersNumberInternal() const
{
    return _connectedToHub ? _peersNum : 0;
}

//==============================================================================

void LnDaemonInterface::subscribeInvoices()
{
    if (!_invoicesSubscribed) {
        InvoiceSubscription invoiceSub;
        auto context = ObserveAsync<Invoice>(this,
            [this](auto invoice) {
                LogCDebug(Lnd) << "subscribeInvoice"
                               << QByteArray::fromStdString(invoice.r_hash()).toHex();
            },
            [this](auto status) { _invoicesSubscribed = false; });

        _grpcClient->makeRpcStreamingRequest(
            &Lightning::Stub::PrepareAsyncSubscribeInvoices, invoiceSub, std::move(context), 0);

        _invoicesSubscribed = true;
    }
}

//==============================================================================

Promise<void> LnDaemonInterface::lndAddNewConnection(QString identityKey)
{
    auto split = identityKey.split(QRegExp("@"), QString::SkipEmptyParts).toVector();
    std::vector<QString> nodeConf(split.begin(), split.end());
    std::string pubKey = nodeConf.at(0).toStdString();
    std::string host = nodeConf.at(1).toStdString();

    ConnectPeerRequest req;
    auto addr = new LightningAddress;
    addr->set_pubkey(pubKey);
    addr->set_host(host);
    req.set_allocated_addr(addr);

    return _grpcClient
        ->makeRpcUnaryRequest<ConnectPeerResponse>(&Lightning::Stub::PrepareAsyncConnectPeer, req)
        .then([] {})
        .tapFail([](grpc::Status status) {
            LogCDebug(Lnd) << "Failed to execute lndAddNewConnection"
                           << status.error_message().c_str();
        })
        .fail([](Status status) {
            if (QString::fromStdString(status.error_message())
                    .contains("already connected to peer")) {
                return QtPromise::resolve();
            } else if (QString::fromStdString(status.error_message())
                    .contains("total outputvalue")) {
                LogCDebug(Lnd) << "Failed to execute connect to peer"
                               << status.error_message().c_str();
                throw std::runtime_error("More than 20% of channel capacity goes to fees. Please choose a lower fee or bigger capacity");
            } else {
                LogCDebug(Lnd) << "Failed to execute connect to peer"
                               << status.error_message().c_str();
                throw std::runtime_error(status.error_message());
            }
        });
}

//==============================================================================

Promise<void> LnDaemonInterface::lndAddNewWatchTower(QString identityKey)
{
    auto split = identityKey.split(QRegExp("@"), QString::SkipEmptyParts).toVector();
    std::vector<QString> nodeConf(split.begin(), split.end());
    std::string pubKey = nodeConf.at(0).toStdString();
    auto bytes = bitcoin::ParseHex(pubKey);
    std::string host = nodeConf.at(1).toStdString();

    wtclientrpc::AddTowerRequest request;
    request.set_pubkey(std::string(bytes.begin(), bytes.end()));
    request.set_address(host);

    return _grpcClient
        ->makeWatchTowerUnaryRequest<wtclientrpc::AddTowerResponse>(
            &wtclientrpc::WatchtowerClient::Stub::PrepareAsyncAddTower, request)
        .then([] {})
        .tapFail([](grpc::Status status) {
            LogCDebug(Lnd) << "Failed to execute lndAddNewWatchTower"
                           << status.error_message().c_str();
        })
        .fail([](Status status) {
            LogCDebug(Lnd) << "Failed to execute connect to watchtower"
                           << status.error_message().c_str();
            throw std::runtime_error(status.error_message());
        });
}

//==============================================================================

Promise<QString> LnDaemonInterface::addChannelRequest(
    QString identityKey, QString localAmountStr, unsigned feeSatsPerByte)
{
    return Promise<QString>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                QStringList nodeConf = identityKey.split(QRegExp("@"), QString::SkipEmptyParts);
                auto pubKey = nodeConf[0];

                int64_t localAmount = localAmountStr.toDouble() * COIN;

                if (!_chainSynced.chain || !_chainSynced.graph) {
                    reject(std::runtime_error("Can't open channel, lnd not synced"));
                    return;
                }

                this->lndAddNewConnection(identityKey)
                    .then([=] { return this->openChannel(pubKey, localAmount, feeSatsPerByte); })
                    .then([resolve](QString outpoint) { resolve(outpoint); })
                    .fail([reject](const std::exception& ex) { reject(std::current_exception()); });
            } catch (const std::exception& ex) {
                LogCDebug(Lnd) << "Failed to execute addChannelRequest "
                               << QString::fromStdString(ex.what());
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<void> LnDaemonInterface::closeAllChannels(unsigned feeSatsPerByte)
{
    return Promise<void>([this, feeSatsPerByte](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            std::vector<Promise<void>> closeAllChannelsPromise;

            for (auto&& activeChannel : _channels) {
                QStringList channelPointInfo
                    = activeChannel.channelOutpoint.split(':', QString::SkipEmptyParts);
                closeAllChannelsPromise.emplace_back(this->closeChannelByFundingTxid(
                    channelPointInfo, !activeChannel.active, feeSatsPerByte));
            }

            QtPromise::all(closeAllChannelsPromise)
                .then([resolve] { resolve(); })
                .fail([reject](Status errStatus) {
                    LogCDebug(Lnd) << " CloseAllChannels fail"
                                   << QString::fromStdString(errStatus.error_message());
                    reject();
                });
        });
    });
}

//==============================================================================

Promise<void> LnDaemonInterface::closeChannel(
    QString channelOutpoint, bool isChannelInactive, unsigned feeSatsPerByte)
{
    return Promise<void>([this, channelOutpoint, isChannelInactive, feeSatsPerByte](
                             const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            QStringList channelPointInfo = channelOutpoint.split(':', QString::SkipEmptyParts);
            this->closeChannelByFundingTxid(channelPointInfo, isChannelInactive, feeSatsPerByte)
                .then([resolve] { resolve(); })
                .fail([reject](Status errStatus) { reject(errStatus.error_message()); });
        });
    });
}

//==============================================================================

auto LnDaemonInterface::decodePayRequest(QString payRequest) -> Promise<LightningPayRequest>
{
    return Promise<LightningPayRequest>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            PayReqString payReq;
            payReq.set_pay_req(payRequest.toStdString());

            _grpcClient
                ->makeRpcUnaryRequest<PayReq>(&Lightning::Stub::PrepareAsyncDecodePayReq, payReq)
                .then([resolve, this](PayReq response) {
                    LightningPayRequest request;
                    request.destination = QString::fromStdString(response.destination());
                    request.paymentHash = QString::fromStdString(response.payment_hash());
                    request.numSatoshis = response.num_satoshis();

                    resolve(request);
                })
                .fail([reject](Status status) {
                    LogCDebug(Lnd)
                        << "Failed to execute decodePayRequest" << status.error_message().c_str();
                    reject(QString::fromStdString(status.error_message()));
                });
        });
    });
}

//==============================================================================

Promise<bool> LnDaemonInterface::verifyHubRoute(int64_t satoshisAmount, bool selfIsSource)
{
    return Promise<bool>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            QStringList nodeConf = _processManager->getNodeConf();
            QtPromise::map(nodeConf.toVector(),
                [satoshisAmount, selfIsSource, this](QString conf, ...) {
                    auto pubkey = conf.split(QChar('@')).at(0);
                    QueryRoutesRequest req;
                    req.set_amt(satoshisAmount);
                    req.set_pub_key(pubkey.toStdString());

                    // go into different direction, from hub to our node
                    if (!selfIsSource) {
                        req.set_pub_key(_infoPubKey.toStdString());
                        req.set_source_pub_key(pubkey.toStdString());
                    }

                    return _grpcClient
                        ->makeRpcUnaryRequest<QueryRoutesResponse>(
                            &Lightning::Stub::PrepareAsyncQueryRoutes, req)
                        .then(
                            [](QueryRoutesResponse response) { return response.routes_size() > 0; })
                        .fail([] { return false; });
                })
                .then([resolve](QVector<bool> values) { resolve(values.contains(true)); })
                .fail(reject);
        });
    });
}

//==============================================================================

Promise<void> LnDaemonInterface::restoreChannelBackups(std::string multiChanBackup)
{
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            RestoreChanBackupRequest req;
            req.set_multi_chan_backup(multiChanBackup);

            _grpcClient
                ->makeRpcUnaryRequest<RestoreBackupResponse>(
                    &Lightning::Stub::PrepareAsyncRestoreChannelBackups, req, 0)
                .then([resolve, reject](RestoreBackupResponse /*response*/) { resolve(); })
                .fail([reject](Status status) {
                    LogCCritical(Lnd) << "Failed to execute restoreChannelBackups"
                                      << status.error_message().c_str();
                    reject();
                    throw;
                });
        });
    });
}

//==============================================================================

Promise<std::string> LnDaemonInterface::invoicePaymentRequestByPreImage(std::string rHash)
{
    return Promise<std::string>([=](const auto& resolve, const auto& reject) mutable {
        QMetaObject::invokeMethod(_executionContext, [=]() mutable {
            PaymentHash req;
            if (rHash.size() == 64) {
                auto parsed = bitcoin::ParseHex(rHash);
                rHash = std::string(parsed.begin(), parsed.end());
            }

            req.set_r_hash(rHash);

            _grpcClient
                ->makeRpcUnaryRequest<Invoice>(&Lightning::Stub::PrepareAsyncLookupInvoice, req)
                .then([resolve](Invoice response) { resolve(response.payment_request()); })
                .fail([reject](grpc::Status status) {
                    LogCCritical(Lnd) << "Failed to execute invoicePaymentRequestByPreImage"
                                      << status.error_message().c_str();
                    reject();
                });
        });
    });
}

//==============================================================================

void LnDaemonInterface::refreshChannels()
{
    QMetaObject::invokeMethod(_executionContext, [this] { lndGetListChannels(); });
}

//==============================================================================

LndGrpcClient* LnDaemonInterface::grpcClient() const
{
    return _grpcClient.get();
}

//==============================================================================

bool LnDaemonInterface::event(QEvent* e)
{
    if (e->type() == QEvent::ThreadChange) {
        QMetaObject::invokeMethod(this,
            [this] {
                _grpcClient->moveToThread(thread());
                _processManager->moveToThread(thread());
            },
            Qt::QueuedConnection);
        return true;
    }
    return QObject::event(e);
}

//==============================================================================

void LnDaemonInterface::tryConnectingToPeers()
{
    QStringList nodeConf = _processManager->getNodeConf();

    _grpcClient
        ->makeRpcUnaryRequest<ListPeersResponse>(
            &Lightning::Stub::PrepareAsyncListPeers, ListPeersRequest())
        .then([](ListPeersResponse response) mutable {
            std::vector<QString> tempListPeersPubkey;
            for (auto&& peer : response.peers()) {
                tempListPeersPubkey.emplace_back(QString::fromStdString(peer.pub_key()));
            }

            return tempListPeersPubkey;
        })
        .then([this, nodeConf](std::vector<QString> peers) {
            for (auto&& conf : nodeConf) {
                QStringList pubKey = conf.split(QRegExp("@"), QString::SkipEmptyParts);

                auto it = std::find(peers.begin(), peers.end(), pubKey.at(0));
                if (it == peers.end()) {
                    lndAddNewConnection(conf);
                }
            }

            this->setConnectedToHub(true);
        })
        .fail([](Status status) {
            LogCDebug(Lnd) << "Failed to execute listPeers" << status.error_message().c_str();
            throw;
        });
}

//==============================================================================

void LnDaemonInterface::tryAddingWatchtowers()
{
    QStringList towersConf = _processManager->getNodeWatchTowers();

    if (!_towersConnected) {
        _grpcClient
            ->makeWatchTowerUnaryRequest<wtclientrpc::ListTowersResponse>(
                &wtclientrpc::WatchtowerClient::Stub::PrepareAsyncListTowers,
                wtclientrpc::ListTowersRequest())
            .then([](wtclientrpc::ListTowersResponse response) mutable {
                std::vector<QString> tempListTowers;

                for (auto&& tower : response.towers()) {
                    tempListTowers.emplace_back(QString::fromStdString(tower.pubkey()));
                }
                return tempListTowers;
            })
            .then([this, towersConf](std::vector<QString> towers) {
                for (auto&& conf : towersConf) {
                    QStringList pubKey = conf.split(QRegExp("@"), QString::SkipEmptyParts);

                    auto it = std::find(towers.begin(), towers.end(), pubKey.at(0));
                    if (it == towers.end()) {
                        lndAddNewWatchTower(conf);
                    }
                }

                this->setTowersConnected(true);
            })
            .fail([](Status status) {
                LogCDebug(Lnd) << "Failed to execute ListTowers" << status.error_message().c_str();
                throw;
            });
    }
}

//==============================================================================

void LnDaemonInterface::doHouseKeeping()
{
    fetchLndInfo();
}

//==============================================================================
