#include "LndChannelsListModel.hpp"
#include <Chain/AbstractChainDataSource.hpp>
#include <Chain/AbstractChainManager.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/BlockHeader.hpp>
#include <Chain/Chain.hpp>
#include <LndTools/LndTypes.hpp>
#include <Models/ChannelRentalHelper.hpp>
#include <Models/ConnextDaemonInterface.hpp>
#include <Models/LnDaemonInterface.hpp>
#include <Models/LnDaemonsManager.hpp>
#include <Models/PaymentNodesManager.hpp>
#include <Networking/AbstractBlockExplorerHttpClient.hpp>
#include <Utils/Logging.hpp>
#include <ViewModels/ApplicationViewModel.hpp>

#include <LndTools/LndTypes.hpp>

#include <boost/optional.hpp>

static const unsigned MIN_TIME = 600; // 10 minutes

//==============================================================================

LndChannelsListModel::LndChannelsListModel(AssetID assetID, AssetsTransactionsCache* assetsTxCache,
    AbstractChainDataSource* chainDataSource, AbstractChainManager* chainManager,
    ChannelRentalHelper* channelRental, PaymentNodeInterface* paymentNodeInterface, QObject* parent)
    : PaymentChannelsListModel(assetID, parent)
    , _paymentNode(qobject_cast<LnDaemonInterface*>(paymentNodeInterface))
    , _chainDataSource(chainDataSource)
    , _channelRentalHelper(channelRental)
{
    Q_ASSERT_X(_paymentNode, __FUNCTION__, "Expecting lnd payment node");
    init(assetsTxCache, chainManager);
}

//==============================================================================

LndChannelsListModel::~LndChannelsListModel() {}

//==============================================================================

int LndChannelsListModel::rowCount(const QModelIndex& parent) const
{
    Q_UNUSED(parent)
    return std::accumulate(std::begin(_channels), std::end(_channels), 0,
        [](const auto& accum, const auto& item) { return accum + item.size(); });
}

//==============================================================================

QVariant LndChannelsListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= rowCount(QModelIndex())) {
        return QVariant();
    }

    auto channel = channelByIndex(index.row());
    auto type = channelType(index.row());

    auto channelBalancesWithoutReserve = BuildChannelsBalance({ channel });

    auto isChannelRental = isRentalChannel(channel.channelOutpoint);

    switch (role) {
    case TotalSatoshisRecvRole:
        return static_cast<double>(channel.totalSatoshisRecv);
    case TotalSatoshisSentRole:
        return static_cast<double>(channel.totalSatoshisSent);
    case RemotePubKeyRole:
        return channel.remotePubKey;
    case LocalBalanceRole:
        return static_cast<double>(
            channel.active ? channelBalancesWithoutReserve.at(0) : channel.localBalance);
    case RemoteBalanceRole:
        return static_cast<double>(
            channel.active ? channelBalancesWithoutReserve.at(1) : channel.remoteBalance);
    case ChannelIdRole:
        return QString::number(channel.channelId);
    case ChannelPointRole:
        return channel.channelOutpoint;
    case CapacityRole:
        return static_cast<double>(channel.capacity);
    case TypeRole:
        return static_cast<int>(type);
    case CSVDelayRole:
        return channel.csvDelay;
    case DetailsRole:
        return channel.details;
    case FundingTxIdRole:
        return getFundingTxId(channel, type);
    case ConfirmationsRole:
        return getConfirmations(channel, type);
    case ExpiresAtRole:
        return static_cast<double>(getExpiresAt(isChannelRental, channel.channelOutpoint));
    case IsRentingChannelRole:
        return isChannelRental;
    case ChannelDateRole: {
        return isChannelRental ? getRentalCreatedDate(channel.channelOutpoint)
                               : _channelsDate.count(channel.channelOutpoint) == 0
                ? 0
                : _channelsDate.at(channel.channelOutpoint);
    }
    default:
        break;
    }

    return QVariant();
}

//==============================================================================

void LndChannelsListModel::refresh()
{
    _paymentNode->refreshChannels();
}

//==============================================================================

QVariantMap LndChannelsListModel::get(QString channelOutpoint)
{
    QVariantMap result;

    int row = 0;
    for (const auto& channels : _channels) {
        auto channelIt = find_if(
            std::begin(channels), std::end(channels), [channelOutpoint](const LndChannel& channel) {
                return channel.channelOutpoint == channelOutpoint;
            });
        if (channelIt != std::end(channels)) {
            row += static_cast<int>(std::distance(std::begin(channels), channelIt));
            QHash<int, QByteArray> names = roleNames();
            QHashIterator<int, QByteArray> it(names);
            while (it.hasNext()) {
                it.next();
                QModelIndex idx = index(row);
                QVariant data = idx.data(it.key());
                result[it.value()] = data;
            }
        }
        row += channels.size();
    }
    return result;
}

//==============================================================================

bool LndChannelsListModel::canOpenRentalChannel()
{
    if (_channelRentalHelper) {
        return _channelRentalHelper->canOpenRentalChannel(_assetID);
    }

    return false;
}

//==============================================================================

bool LndChannelsListModel::canExtendRentalChannel(double expiresAtTimeSeconds)
{
    return expiresAtTimeSeconds - QDateTime::currentSecsSinceEpoch() > MIN_TIME;
}

//==============================================================================

unsigned LndChannelsListModel::recentPendingConfirmation() const
{
    return _recentPendingConfirmation;
}

//==============================================================================

void LndChannelsListModel::onUpdateChannels(std::vector<LndChannel> channels, ChannelType type)
{
    auto i = static_cast<size_t>(type);
    auto& subChannels = _channels.at(i);

    if (subChannels != channels) {
        beginResetModel();
        _channels[i] = channels;
        endResetModel();
        countChanged();
    }

    for (size_t j = 0; j < _channels.at(i).size(); ++j) {
        const auto& channel = _channels.at(i).at(j);
        if (_confirmations.count({ channel.channelOutpoint, type }) == 0) {
            _confirmations.emplace(std::make_pair(channel.channelOutpoint, type), 0);
            getChannelHeight(channel, type);
        }

        if (type == ChannelType::ForceClosing) {
            updateForceCloseDetails();
        }
    }

    updateChannelsDate(type);
}

//==============================================================================

void LndChannelsListModel::updateConfirmations()
{
    if (_chainView) {
        QPointer<LndChannelsListModel> self{ this };
        _chainView->chainHeight().then([self](size_t newHeight) {
            if (self) {
                self->_tipHeight = newHeight;
                self->dataChanged(self->index(0), self->index(self->rowCount(QModelIndex()) - 1));
                self->findRecentPendingConfirmation();
            }
        });
    }
}

//==============================================================================

void LndChannelsListModel::onChainHeightChanged()
{
    for (auto pair : _confirmations) {
        if (pair.second == 0) {
            auto channelOutpoint = pair.first.first;
            auto channelType = pair.first.second;
            // std::array<Channels, 5> _channels; // { Active, Inactive, Pending, Closing,
            // ForceClosing }, 2 = Pending
            auto channels = _channels.at(static_cast<size_t>(channelType));

            auto it = std::find_if(
                channels.begin(), channels.end(), [channelOutpoint](LndChannel channel) {
                    return channel.channelOutpoint == channelOutpoint;
                });

            if (it != channels.end()) {
                getChannelHeight(*it, channelType);
            }
        }
    }
    updateConfirmations();
}

//==============================================================================

void LndChannelsListModel::updateForceCloseDetails()
{
    for (auto&& channel : _channels.at(4)) {
        channel.details["maturity_blocks"] = channel.details["maturity_height"].toUInt()
            - _confirmations.at({ channel.channelOutpoint, ChannelType::ForceClosing }) + 1;
    }
    dataChanged(index(0), index(rowCount(QModelIndex()) - 1), { DetailsRole });
}

//==============================================================================

void LndChannelsListModel::updateRentalChannels(AssetID assetID, QString channelOutpoint)
{
    if (_assetID == assetID) {
        dataChanged(
            index(0), index(rowCount(QModelIndex()) - 1), { ExpiresAtRole, IsRentingChannelRole });
    }
}

//==============================================================================

LndChannel LndChannelsListModel::channelByIndex(int row) const
{
    int rowAccum = 0;
    for (size_t i = 0; i < _channels.size(); ++i) {
        const auto& subChannels = _channels.at(i);
        for (size_t j = 0; j < subChannels.size(); ++j) {
            if (rowAccum + j == row) {
                return subChannels.at(j);
            }
        }
        rowAccum += subChannels.size();
    }

    boost::optional<LndChannel> lndChannel;
    return lndChannel.get();
}

//==============================================================================

LndChannelsListModel::ChannelType LndChannelsListModel::channelType(int row) const
{
    // 3, 4, 0, 2
    // total = 9
    // channelType(0) == Active
    // channelType(2) == Active
    // channelType(3) == InActive
    // channelType(4) == InActive
    // channelType(7) == Closing
    int rowAccum = 0;
    for (size_t i = 0; i < _channels.size(); ++i) {
        rowAccum += _channels.at(i).size();
        if (row < rowAccum) {
            return static_cast<ChannelType>(i);
        }
    }

    return ChannelType::Undefined;
}

//==============================================================================

QString LndChannelsListModel::getFundingTxId(LndChannel channel, ChannelType type) const
{
    return type == ChannelType::ForceClosing || type == ChannelType::Closing
        ? channel.details.value("closing_txid").toString()
        : channel.channelOutpoint.split(":").first();
}

//==============================================================================

int LndChannelsListModel::getConfirmations(LndChannel channel, ChannelType type) const
{
    if (_confirmations.count({ channel.channelOutpoint, type }) > 0) {
        auto channelConfirmHeight = _confirmations.at({ channel.channelOutpoint, type });
        return (_tipHeight > 0 && channelConfirmHeight > 0)
            ? static_cast<int>(_tipHeight - channelConfirmHeight + 1)
            : 0;
    }

    return 0;
}

//==============================================================================

void LndChannelsListModel::getChannelHeight(
    LndChannel channel, LndChannelsListModel::ChannelType type)
{
    QPointer<LndChannelsListModel> self(this);
    _chainDataSource->getRawTransaction(_assetID, getFundingTxId(channel, type))
        .then([self, channel, type](Wire::TxConfrimation confirmation) {
            if (!self) {
                return;
            }
            auto it = self->_confirmations.find({ channel.channelOutpoint, type });
            if (it != std::end(self->_confirmations) && !confirmation.blockHash.empty()) {
                it->second = confirmation.blockHeight;
            }
            self->updateConfirmations();
            if (type == ChannelType::ForceClosing) {
                self->updateForceCloseDetails();
            }
        })
        .fail([channel, type, self](const std::exception& ex) {
            LogCDebug(Api) << "Failed to get raw transaction" << ex.what();
            if (!self) {
                return;
            }
            self->_confirmations.erase({ channel.channelOutpoint, type });
        });
}

//==============================================================================

void LndChannelsListModel::getChannelDate(QString outpoint, QPersistentModelIndex index)
{
    QPointer<LndChannelsListModel> self(this);

    if (_transactionsCache) {

        auto txId = outpoint.split(':', Qt::SkipEmptyParts).front();

        _transactionsCache->transactionById(txId)
            .then([self, txId, index, outpoint](std::shared_ptr<OnChainTx> onChainTx) {
                if (!self) {
                    return;
                }

                auto it = self->_channelsDate.find(outpoint);

                if (it == std::end(self->_channelsDate)) {
                    return;
                }

                it->second = onChainTx->transactionDate().toMSecsSinceEpoch();

                if (index.isValid()) {
                    self->dataChanged(index, index);
                }
            })
            .fail([self, txId] {
                if (!self) {
                    return;
                }

                self->_channelsDate.erase(txId);
            });
    }
}

//==============================================================================

bool LndChannelsListModel::isRentalChannel(QString channelOutpoint) const
{
    return _channelRentalHelper->isRental(_assetID, orderbook::ChannelRentingManager::ChannelType::LND, channelOutpoint);
}

//==============================================================================

uint64_t LndChannelsListModel::getExpiresAt(bool isRental, QString channelOutpoint) const
{
    if (isRental) {
        return _channelRentalHelper->expiresTime(_assetID, orderbook::ChannelRentingManager::ChannelType::LND, channelOutpoint);
    }
    return 0;
}

//==============================================================================

uint64_t LndChannelsListModel::getRentalCreatedDate(QString channelOutpoint) const
{
    return _channelRentalHelper->createdTime(_assetID, orderbook::ChannelRentingManager::ChannelType::LND, channelOutpoint);
}

//==============================================================================

void LndChannelsListModel::setRecentPendingConfirmation(unsigned value)
{
    if (_recentPendingConfirmation != value) {
        _recentPendingConfirmation = value;
        recentPendingConfirmationChanged();
    }
}

//==============================================================================

void LndChannelsListModel::findRecentPendingConfirmation()
{
    auto channels = _channels.at(static_cast<size_t>(2)); // Pending channels
    if (channels.size() > 0) {
        std::vector<unsigned> confirmations;
        for (auto& channel : channels) {
            confirmations.push_back(getConfirmations(channel, ChannelType::Pending));
        }
        auto it = std::min_element(confirmations.begin(), confirmations.end());
        if (it != confirmations.end()) {
            setRecentPendingConfirmation(*it);
        }
    }
}

//==============================================================================

void LndChannelsListModel::updateChannelsDate(LndChannelsListModel::ChannelType type)
{
    if (!_transactionsCache) {
        return;
    }

    int startIndex
        = type == LndChannelsListModel::ChannelType::Undefined ? 0 : static_cast<int>(type);
    int endIndex
        = type == LndChannelsListModel::ChannelType::Undefined ? _channels.size() : startIndex + 1;
    int modelIndex = 0;

    for (int i = startIndex; i < endIndex; ++i) {
        auto& subChannels = _channels.at(i);

        for (size_t j = 0; j < subChannels.size(); ++j) {
            const auto& channel = subChannels.at(j);
            QStringList channelPointInfo
                = channel.channelOutpoint.split(':', QString::SkipEmptyParts);

            if (_channelsDate.count(channel.channelOutpoint) == 0 && !channelPointInfo.isEmpty()) {
                _channelsDate.emplace(channel.channelOutpoint, 0);
                getChannelDate(channel.channelOutpoint, index(modelIndex + j));
            }
        }

        modelIndex += subChannels.size();
    }
}

//==============================================================================

void LndChannelsListModel::initPaymentNode()
{
    connect(_paymentNode, &LnDaemonInterface::channelsChanged, this,
        [this](std::vector<LndChannel> active, std::vector<LndChannel> inactive) {
            onUpdateChannels(active, ChannelType::Active);
            onUpdateChannels(inactive, ChannelType::Inactive);
        });

    connect(_paymentNode, &LnDaemonInterface::pendingChannelsChanged, this,
        std::bind(static_cast<void (LndChannelsListModel::*)(std::vector<LndChannel>, ChannelType)>(
                      &LndChannelsListModel::onUpdateChannels),
            this, std::placeholders::_1, ChannelType::Pending));

    connect(_paymentNode, &LnDaemonInterface::pendingWaitingCloseChannelsChanged, this,
        std::bind(static_cast<void (LndChannelsListModel::*)(std::vector<LndChannel>, ChannelType)>(
                      &LndChannelsListModel::onUpdateChannels),
            this, std::placeholders::_1, ChannelType::Closing));

    connect(_paymentNode, &LnDaemonInterface::pendingForceClosingChannelsChanged, this,
        std::bind(static_cast<void (LndChannelsListModel::*)(std::vector<LndChannel>, ChannelType)>(
                      &LndChannelsListModel::onUpdateChannels),
            this, std::placeholders::_1, ChannelType::ForceClosing));

    QPointer<LndChannelsListModel> self{ this };

    _paymentNode->channels().then([self](LndChannels channels) {
        if (self) {
            self->onUpdateChannels(channels, ChannelType::Active);
        }
    });

    _paymentNode->inactiveChannels().then([self](LndChannels channels) {
        if (self) {
            self->onUpdateChannels(channels, ChannelType::Inactive);
        }
    });

    _paymentNode->pendingChannels().then([self](LndChannels channels) {
        if (self) {
            self->onUpdateChannels(channels, ChannelType::Pending);
        }
    });

    _paymentNode->pendingWaitingCloseChannels().then([self](LndChannels channels) {
        if (self) {
            self->onUpdateChannels(channels, ChannelType::Closing);
        }
    });

    _paymentNode->pendingForceClosingChannels().then([self](LndChannels channels) {
        if (self) {
            self->onUpdateChannels(channels, ChannelType::ForceClosing);
        }
    });
}

//==============================================================================

void LndChannelsListModel::init(
    AssetsTransactionsCache* txCache, AbstractChainManager* chainManager)
{
    QPointer<LndChannelsListModel> self(this);
    chainManager
        ->getChainView(_assetID, AbstractChainManager::ChainViewUpdatePolicy::CompressedEvents)
        .then([self](std::shared_ptr<ChainView> chainView) {
            if (!self) {
                return;
            }
            self->_chainView.swap(chainView);
            connect(self->_chainView.get(), &ChainView::bestBlockHashChanged, self,
                &LndChannelsListModel::onChainHeightChanged);
            self->onChainHeightChanged();
        });
    if (_channelRentalHelper) {
        connect(_channelRentalHelper, &ChannelRentalHelper::channelAdded, this,
            &LndChannelsListModel::updateRentalChannels);
        connect(_channelRentalHelper, &ChannelRentalHelper::channelChanged, this,
            &LndChannelsListModel::updateRentalChannels);
    }

    _channelsDate.clear();

    txCache->cacheById(_assetID).then([self](AbstractTransactionsCache* cache) {
        if (!self) {
            return;
        }
        self->_transactionsCache = cache;
        self->updateChannelsDate();
    });

    initPaymentNode();
}

//==============================================================================
