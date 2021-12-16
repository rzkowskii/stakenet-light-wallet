#include "ConnextDaemonInterface.hpp"
#include <Data/WalletAssetsModel.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <EthCore/Types.hpp>
#include <LndTools/AbstractConnextApi.hpp>
#include <LndTools/ConnextHttpClient.hpp>
#include <LndTools/ConnextProcessManager.hpp>
#include <LndTools/ConnextTypes.hpp>
#include <Networking/DockerApiClient.hpp>
#include <Networking/LocalSocketRequestHandlerImpl.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Tools/Common.hpp>
#include <Utils/Logging.hpp>

//==============================================================================

QString FailureMsg(QString fullMessage, QString action)
{
    if (fullMessage.contains("cannot estimate gas")) {
        return QString("Cannot %1. Not enough gas reserved when using send.").arg(action);
    } else if (fullMessage.contains("could not detect network")) {
        return QString("Cannot %1. Could not detect network.").arg(action);
    } else {
        return (fullMessage);
    }
}

//==============================================================================

ConnextDaemonInterface::ConnextDaemonInterface(AbstractConnextApi* connextClient,
    ConnextProcessManager* processManager, const WalletAssetsModel& assetsModel, AssetID assetID,
    QObject* parent)
    : PaymentNodeInterface(Enums::PaymentNodeType::Connext, parent)
    , _executionContext(new QObject(this))
    , _connextClient(connextClient)
    , _processManager(processManager)
    , _assetID(assetID)
    , _assetsModel(assetsModel)
{
    init();
}

//==============================================================================

ConnextDaemonInterface::~ConnextDaemonInterface()
{
    _channelsUpdateTimer->stop();
}

//==============================================================================

Promise<std::vector<ConnextChannel>> ConnextDaemonInterface::channels() const
{
    return Promise<std::vector<ConnextChannel>>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_channels); });
    });
}

//==============================================================================

Promise<QString> ConnextDaemonInterface::identifier() const
{
    return Promise<QString>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_identifier); });
    });
}

//==============================================================================

Promise<void> ConnextDaemonInterface::closeAllChannels(unsigned feeSatsPerByte)
{
    return Promise<void>([this, feeSatsPerByte](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(); });
    });
}

//==============================================================================

AbstractPaymentNodeProcessManager* ConnextDaemonInterface::processManager() const
{
    return _processManager;
}

//==============================================================================

Promise<bool> ConnextDaemonInterface::isChannelsOpened() const
{
    return Promise<bool>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_channels.size() > 0); });
    });
}

//==============================================================================

void ConnextDaemonInterface::refreshChannels()
{
    QMetaObject::invokeMethod(_executionContext, [this] { connextGetListChannels(); });
}

//==============================================================================

void ConnextDaemonInterface::connextGetListChannels()
{
    connextGetActiveChannels();
}

//==============================================================================

Promise<QString> ConnextDaemonInterface::openChannel(
    QString identityKey, int64_t amount, AssetID assetID) const
{
    return Promise<QString>([this, identityKey, amount, assetID](
                                const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto asset = _assetsModel.assetById(assetID);
            auto chainId = asset.params().chainId.value();

            _connextClient->getChannelsList(_identifier)
                .then([identityKey, chainId, reject](QVector<QVariantMap> channels) {
                    for (auto&& channel : channels) {
                        auto chain
                            = channel.value("networkContext").toMap().value("chainId").toInt();
                        if ((channel.value("aliceIdentifier").toString() == identityKey
                                && chainId == chain)
                            || (channel.value("bobIdentifier").toString() == identityKey
                                   && chainId == chain)) {
                            return reject(QString("Channel to specified node already exists, "
                                                  "you can deposit funds to start using it."));
                        }
                    }
                });

            auto convertedAmount
                = eth::ConvertDenominations(eth::u256{ amount }, 8, UNITS_PER_CURRENCY.at(assetID));

            auto contractAddress
                = asset.token() ? asset.token()->contract() : asset.connextData().tokenAddress;

            QVariantMap setupPayload;
            setupPayload["publicIdentifier"] = _identifier;
            setupPayload["counterpartyIdentifier"] = identityKey;
            setupPayload["chainId"] = chainId;
            setupPayload["timeout"] = "172800"; // TODO: set timeout

            _connextClient->setupChannel(setupPayload)
                .then([this, convertedAmount, assetID, amount, resolve, reject](QString channelAddress) {
                    LogCDebug(Connext) << "Setup channel finised!";

                    if(amount == 0){
                         resolve(channelAddress);
                    }

                    deposit(channelAddress, convertedAmount, assetID)
                        .then([resolve](QString channelAddress) { resolve(channelAddress); })
                        .fail([reject](const ConnextApiException& ex) {
                            LogCDebug(Connext) << "Connext deposit channel failed!" << ex.name;
                            reject(FailureMsg(ex.name, "deposit channel"));
                        });
                })
                .fail([reject](const ConnextApiException& ex) {
                    LogCDebug(Connext) << "Connext channel did not open!" << ex.name;
                    reject(ex.msg);
                })
                .fail([reject](const std::exception& ex) { reject(std::current_exception()); })
                .fail([reject]() {
                    LogCDebug(Connext) << "Connext channel did not open!"; //<< ex.name;
                    reject(std::current_exception());
                });
        });
    });
}

//==============================================================================

AbstractConnextApi* ConnextDaemonInterface::httpClient() const
{
    return _connextClient;
}

//==============================================================================

Promise<QString> ConnextDaemonInterface::depositChannel(
    int64_t amount, QString channelAddress, AssetID assetID) const
{
    return Promise<QString>(
        [this, amount, assetID, channelAddress](const auto& resolve, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                auto convertedAmount = eth::ConvertDenominations(
                    eth::u256{ amount }, 8, UNITS_PER_CURRENCY.at(assetID));

                deposit(channelAddress, convertedAmount, assetID)
                    .then([resolve](QString channelAddress) { resolve(channelAddress); })
                    .fail([reject](const ConnextApiException& ex) {
                        LogCDebug(Connext) << "Connext deposit channel failed!" << ex.name;

                        reject(FailureMsg(ex.name, "deposit channel"));
                    })
                    .fail([reject](const std::exception& ex) { reject(std::current_exception()); });
            });
        });
}

//==============================================================================

Promise<void> ConnextDaemonInterface::withdraw(
    QString recipientAddress, int64_t amount, QString channelAddress, AssetID assetID) const
{
    auto asset = _assetsModel.assetById(assetID);

    auto convertedAmount
        = eth::ConvertDenominations(eth::u256{ amount }, 8, UNITS_PER_CURRENCY.at(assetID));

    auto contractAddress
        = asset.token() ? asset.token()->contract() : asset.connextData().tokenAddress;

    QVariantMap payload;
    payload["publicIdentifier"] = _identifier;
    payload["channelAddress"] = channelAddress;
    payload["amount"] = QString::fromStdString(convertedAmount.str());
    payload["assetId"] = contractAddress;
    payload["recipient"] = recipientAddress;
    payload["fee"] = "0";

    QVariantMap meta;
    meta["initiatorSubmits"] = QVariant::fromValue(true);
    payload["meta"] = meta;

    qDebug() << "withdraw payload";
    qDebug() << "publicIdentifier" << _identifier;
    qDebug() << "channelAddress" << channelAddress;
    qDebug() << "amount" << QString::fromStdString(convertedAmount.str());
    qDebug() << "assetId" << assetID;
    qDebug() << "recipientAddress" << recipientAddress;

    return Promise<void>([this, payload](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            _connextClient->withdraw(payload)
                .then([resolve]() { resolve(); })
                .fail([reject](const ConnextApiException& ex) {
                    LogCDebug(Connext) << "Connext withdraw failed!" << ex.name;
                    reject(ex.msg);
                });
        });
    });
}

//==============================================================================

Promise<ConnextDaemonInterface::ConnextBalance> ConnextDaemonInterface::channelsBalance() const
{
    return Promise<ConnextBalance>([this](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] { resolve(_channelsBalance); });
    });
}

//==============================================================================

void ConnextDaemonInterface::init()
{
    auto asset = _assetsModel.assetById(_assetID);
    _tokenAddress = asset.token() ? asset.token()->contract() : asset.connextData().tokenAddress;
    _channelsUpdateTimer = new QTimer(this);
    _channelsUpdateTimer->setSingleShot(false);
    _channelsUpdateTimer->setInterval(15 * 1000);
    _channelsUpdateTimer->start();

    connect(_channelsUpdateTimer, &QTimer::timeout, this, &ConnextDaemonInterface::refreshChannels);
    connect(_processManager, &ConnextProcessManager::runningChanged, this, [this]() {
        if (!_processManager->running()) {
            _channelsBalance = ConnextBalance();
            _channels.clear();
        }
    });
}

//==============================================================================

void ConnextDaemonInterface::setChannelsBalance(const ConnextBalance& newBalance)
{
    if (_channelsBalance != newBalance) {
        _channelsBalance = newBalance;
        channelsBalanceChanged(_channelsBalance);
    }
}

//==============================================================================

Promise<QString> ConnextDaemonInterface::reconcileChannel(
    QString channelAddress, AssetID assetID) const
{
    return Promise<QString>(
        [this, assetID, channelAddress](const auto& resolve, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                auto asset = _assetsModel.assetById(assetID);

                auto contractAddress
                    = asset.token() ? asset.token()->contract() : asset.connextData().tokenAddress;

                reconcile(channelAddress, contractAddress)
                    .then([this, resolve](QString channelAddress) { resolve(channelAddress); })
                    .fail([reject](const ConnextApiException& ex) {
                        qDebug(Connext) << "Connext reconcile channel failed!" << ex.name;
                        reject(ex.msg);
                    })
                    .fail([reject](const std::exception& ex) { reject(std::current_exception()); });
            });
        });
}

//==============================================================================

Promise<QString> ConnextDaemonInterface::restoreChannel(AssetID assetID) const
{
    return Promise<QString>([this, assetID](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto asset = _assetsModel.assetById(assetID);
            auto chainId = asset.params().chainId.value();

            QVariantMap payload;
            payload["counterpartyIdentifier"]
                = "vector6At5HhbfhcE1p1SBTxAL5ej71AGWkSSxrSTmdj6wuFHquJ1hg8";
            payload["chainId"] = chainId;

            _connextClient->restoreState(payload)
                .then([resolve](QString channelAddress) { resolve(channelAddress); })
                .fail([reject](const ConnextApiException& ex) {
                    LogCDebug(Connext) << "Connext restore failed!" << ex.name;
                    reject(ex.msg);
                })
                .fail([reject]() { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

void ConnextDaemonInterface::connextGetActiveChannels()
{
    QPointer<ConnextDaemonInterface> self{ this };
    isActive().then([self](bool result) {
        if (result && !self->_identifier.isEmpty()) {
            self->_connextClient->getChannelsList(self->_identifier)
                .then([self](QVector<QVariantMap> channels) {
                    std::vector<ConnextChannel> channelsVector;

                    for (auto&& channel : channels) {
                        ConnextChannel chan = ConnextChannel::FromVariantChannelConnextChannel(
                            channel, self->_identifier, self->_tokenAddress);
                        channelsVector.push_back(chan);
                    }

                    auto newBalance = self->_channelsBalance;
                    auto balances = BuildConnextChannelsBalance(channelsVector);

                    auto sourceDecimals = UNITS_PER_CURRENCY.at(self->_assetID);
                    newBalance.localBalance
                        = eth::ConvertDenominations(balances[0], sourceDecimals, 8);
                    newBalance.remoteBalance
                        = eth::ConvertDenominations(balances[1], sourceDecimals, 8);

                    self->setChannelsBalance(newBalance);

                    self->_channels.swap(channelsVector);
                    self->channelsChanged(self->_channels);
                    self->hasChannelChanged(!self->_channels.empty());
                });
        }
    });
}

//==============================================================================

Promise<QString> ConnextDaemonInterface::deposit(
    QString channelAddress, eth::u256 amount, AssetID assetID) const
{
    return Promise<QString>(
        [this, channelAddress, amount, assetID](const auto& resolve, const auto& reject) {
            const auto& asset = _assetsModel.assetById(assetID);
            AbstractConnextApi::DepositTxParams sendDepositPayload;
            sendDepositPayload.assetID = assetID;
            sendDepositPayload.channelAddress = channelAddress;
            sendDepositPayload.amount = QString::fromStdString(amount.str());
            sendDepositPayload.tokenAddress = asset.connextData().tokenAddress;
            sendDepositPayload.chainId = *asset.params().chainId;
            sendDepositPayload.publicIdentifier = _identifier;

            qDebug() << "Deposit payload";
            qDebug() << "channelAddress" << channelAddress;
            qDebug() << "amount" << QString::fromStdString(amount.str());
            qDebug() << "assetId" << assetID;
            qDebug() << "chainId" << QVariant::fromValue(sendDepositPayload.chainId);
            qDebug() << "publicIdentifier" << _identifier;

            //        LogCDebug(Connext) << "Send deposit tx request:" << sendDepositPayload;

            _connextClient->sendDeposit(sendDepositPayload)
                .then([this, channelAddress, tokenAddress = sendDepositPayload.tokenAddress,
                          resolve, reject]() {
                    LogCDebug(Connext) << "Send deposit tx finised!";
                    resolve(channelAddress);
                })
                .fail([reject](const ConnextApiException& ex) {
                    LogCDebug(Connext) << "Connext deposit failed!" << ex.name;
                    reject(std::current_exception());
                })
                .fail([reject](const std::exception& ex) { reject(std::current_exception()); });
        });
}
//==============================================================================

Promise<QString> ConnextDaemonInterface::reconcile(
    QString channelAddress, QString contractAddress) const
{
    return Promise<QString>(
        [this, channelAddress, contractAddress](const auto& resolve, const auto& reject) {
            QVariantMap depositPayload;
            depositPayload["channelAddress"] = channelAddress;
            depositPayload["publicIdentifier"] = _identifier;
            depositPayload["assetId"] = contractAddress;

            qDebug() << "reconcile payload";
            qDebug() << "channelAddress" << channelAddress;
            qDebug() << "publicIdentifier" << _identifier;
            qDebug() << "assetId" << contractAddress;

            _connextClient->reconcile(depositPayload)
                .then([this, resolve](QString channelAddress) {
                    qDebug() << "Reconcile finished! Channel address:" << channelAddress;
                    resolve(channelAddress);
                })
                .fail([reject](const ConnextApiException& ex) {
                    qDebug() << "Reconcile failed!" << ex.name;
                    reject(std::current_exception());
                })
                .fail([reject](const std::exception& ex) { reject(std::current_exception()); });
    });
}

//==============================================================================

Promise<bool> ConnextDaemonInterface::verifyHubRoute(int64_t satoshisAmount, bool selfIsSource)
{
    return Promise<bool>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            std::vector<ConnextChannel> channelsVector;
            _connextClient->getChannelsList(_identifier)
                .then([this, satoshisAmount, selfIsSource, &channelsVector](
                          QVector<QVariantMap> channels) {
                    for (auto&& channel : channels) {
                        ConnextChannel connextChannel
                            = ConnextChannel::FromVariantChannelConnextChannel(
                                channel, _identifier, _tokenAddress);
                        channelsVector.push_back(connextChannel);
                    }
                })
                .wait();

            QStringList nodeConf = _processManager->getNodeConf();
            QtPromise::map(nodeConf.toVector(),
                [satoshisAmount, selfIsSource, channelsVector, this](QString pubkey, ...) {
                    for (auto&& channel : channelsVector) {
                        if (channel.remotePubKey == pubkey) {
                            if (selfIsSource) {
                                return channel.localBalance > satoshisAmount;
                            } else {
                                return channel.remoteBalance > satoshisAmount;
                            }
                        }
                    }
                })
                .then([resolve](QVector<bool> values) { resolve(values.contains(true)); })
                .fail(reject);
        });
    });
}

//==============================================================================

void ConnextDaemonInterface::fetchInfo()
{
    if (_identifier.isEmpty()) {
        _connextClient->getPublicIdentifier().then([this](QString identifier) {
            _identifier = identifier;
            identifierChanged(_identifier);

            //            QVariantMap payload;
            //            payload["counterpartyIdentifier"] =
            //            "vector5wTFqAqyphMqbt5dWBoEt6KrkqBRzQeLAHEoV71HfrAcseukvr";
            //            payload["chainId"] = 1;

            //            _connextClient->restoreState(payload)
            //                .then([this](QString channelAddress) {
            //                    qDebug() << "channel ADRESS" << channelAddress;
            ////                    resolve(channelAddress);

            //                })
            //                .fail([](const ConnextApiException& ex) {
            //                    qDebug() << "restoreState failed!" << ex.name;

            //                    LogCDebug(Connext) << "Connext channel did not open!" << ex.name;
            ////                    reject(ex.name);
            //                }
            //                    );
        });
    }
}

//==============================================================================

Promise<bool> ConnextDaemonInterface::isActive() const
{
    if (_nodeInitialized) {
        return _connextClient->getPublicIdentifier()
            .then([](QString identifier) { return !identifier.isEmpty(); })
            .fail([]() { return false; });
    }
    return QtPromise::resolve(false);
}

//==============================================================================

void ConnextDaemonInterface::setNodeInitialized(bool value)
{
    if (value != _nodeInitialized) {
        _nodeInitialized = value;
        if (_nodeInitialized) {
            fetchInfo();
        }
    }
}

//==============================================================================
