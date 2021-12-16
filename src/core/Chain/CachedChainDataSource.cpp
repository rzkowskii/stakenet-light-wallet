#include "CachedChainDataSource.hpp"
#include <Chain/AbstractChainManager.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/Chain.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/AbstractNetworkingFactory.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Utils/Logging.hpp>
#include <streams.h>
#include <transaction.h>
#include <utilstrencodings.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <boost/range/algorithm.hpp>

//==============================================================================

CachedChainDataSource::CachedChainDataSource(AbstractNetworkingFactory& networkingFactory,
    const WalletAssetsModel& assetsModel, AssetsTransactionsCache& transactionsCache,
    const AbstractMutableChainManager& chainManager, QObject* parent)
    : AbstractChainDataSource(parent)
    , _executionContext(new QObject(this))
    , _networkingFactory(networkingFactory)
    , _assetsModel(assetsModel)
    , _transactionsCache(transactionsCache)
    , _chainManager(chainManager)
    , _blockHeadersCache(10000)
    , _rawTranscationsCache(1000)
{
}

//==============================================================================

Promise<std::vector<Wire::VerboseBlockHeader>> CachedChainDataSource::getBlockHeaders(
    AssetID assetID, BlockHash startingBlockHash) const
{
    return Promise<std::vector<Wire::VerboseBlockHeader>>(
        [=](const auto& resolve, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                auto syncHelper = &this->chainSyncHelper(assetID);
                syncHelper->getLastHeaders(startingBlockHash)
                    .then([this, resolve, assetID](std::vector<Wire::VerboseBlockHeader> headers) {
                        boost::for_each(headers, [this, assetID](const auto& header) {
                            _blockHeadersCache.insert(BlockHeadersCache::key_type{ assetID,
                                                          QString::fromStdString(header.hash) },
                                header);
                        });

                        resolve(headers);
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            });
        });
}

//==============================================================================

Promise<Wire::VerboseBlockHeader> CachedChainDataSource::getBlockHeader(
    AssetID assetID, BlockHash hash) const
{
    return Promise<Wire::VerboseBlockHeader>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto cacheKey = BlockHeadersCache::key_type{ assetID, hash };
                if (auto opt = this->lookupHeaderFromCache(assetID, hash)) {
                    resolve(*opt);
                    return;
                }

                auto client = this->apiClient(assetID);
                client->getBlockHeader(hash)
                    .then([resolve, cacheKey, this](QByteArray result) {
                        QJsonObject obj = QJsonDocument::fromJson(result).object();
                        auto header = Wire::VerboseBlockHeader::FromJson(obj);
                        _blockHeadersCache.insert(cacheKey, header);
                        resolve(header);
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<BlockHash> CachedChainDataSource::getBlockHash(AssetID assetID, size_t blockHeight) const
{
    return Promise<BlockHash>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto client = this->apiClient(assetID);

                if (auto header = _chainManager.chainById(assetID).headerAt(blockHeight)) {
                    auto hash = QString::fromStdString(header->hash);
                    _blockHeadersCache.insert(
                        BlockHeadersCache::key_type{ assetID, hash }, header.get());
                    resolve(hash);
                    return;
                }

                if (_secondLayerBlockHashIndex.count(assetID) > 0
                    && _secondLayerBlockHashIndex.at(assetID).count(blockHeight) > 0) {
                    resolve(_secondLayerBlockHashIndex.at(assetID).at(blockHeight));
                    return;
                }

                client->getBlockHashByHeight(blockHeight)
                    .then([=](QByteArray bytes) {
                        QJsonObject obj = QJsonDocument::fromJson(bytes).object();
                        auto result = Wire::VerboseBlockHeader::FromJson(obj);
                        _blockHeadersCache.insert(BlockHeadersCache::key_type{ assetID,
                                                      QString::fromStdString(result.hash) },
                            result);
                        resolve(QString::fromStdString(result.hash));
                    })
                    .fail([reject]() { reject(std::current_exception()); });

            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<std::tuple<BlockHash, BlockHeight>> CachedChainDataSource::getBestBlockHash(
    AssetID assetID) const
{
    return Promise<std::tuple<BlockHash, BlockHeight>>(
        [=](const auto& resolve, const auto& reject) {
            QMetaObject::invokeMethod(_executionContext, [=] {
                this->apiClient(assetID)
                    ->getBestBlockHash()
                    .then([=](QByteArray response) {
                        QJsonDocument doc = QJsonDocument::fromJson(response);
                        QJsonArray data = doc.array();
                        auto obj = data.first().toObject();
                        resolve(std::make_tuple(obj.value("hash").toString(),
                            static_cast<size_t>(obj.value("height").toInt())));
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            });
        });
}

//==============================================================================

Promise<std::vector<unsigned char>> CachedChainDataSource::getBlock(
    AssetID assetID, BlockHash hash) const
{
    return Promise<std::vector<unsigned char>>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto client = this->apiClient(assetID);

                client->getBlock(hash)
                    .then([client, resolve, this](QByteArray rawBlock) {
                        auto blockHex = QJsonDocument::fromJson(rawBlock)
                                            .object()
                                            .value("hex")
                                            .toString()
                                            .toStdString();
                        resolve(bitcoin::ParseHex(blockHex));
                    })
                    .fail(
                        [reject](const NetworkUtils::ApiErrorException& error) { reject(error); });
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<Wire::StrippedBlock> CachedChainDataSource::getLightWalletBlock(
    AssetID assetID, BlockHash hash, int64_t blockHeight) const
{
    return Promise<Wire::StrippedBlock>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            this->chainSyncHelper(assetID)
                .getLightWalletBlock(hash, blockHeight)
                .then([=](Wire::StrippedBlock block) { resolve(block); })
                .fail([reject]() { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

Promise<std::vector<std::string>> CachedChainDataSource::getFilteredBlock(
    AssetID assetID, BlockHash hash) const
{
    return Promise<std::vector<std::string>>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto client = this->apiClient(assetID);

                _transactionsCache.cacheByIdSync(assetID)
                    .transactionsInBlock(hash)
                    .then([client, this](const std::vector<QString>& transactions) {
                        return QtPromise::map(transactions, [client, this](const QString& id, ...) {
                            return client->getRawTransaction(id);
                        });
                    })
                    .then([resolve](const QVector<QByteArray>& rawTransactions) mutable {
                        std::vector<std::string> txns;
                        for (auto&& rawTxBytes : rawTransactions) {
                            auto rawTx = Wire::TxConfrimation::FromJson(
                                QJsonDocument::fromJson(rawTxBytes).object());
                            txns.emplace_back(rawTx.hexTx);
                        }

                        resolve(txns);
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<Wire::EncodedBlockFilter> CachedChainDataSource::getBlockFilter(
    AssetID assetID, BlockHash hash) const
{
    return Promise<Wire::EncodedBlockFilter>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                if (auto opt = this->lookupHeaderFromCache(assetID, hash)) {
                    resolve(opt.get().filter);
                    return;
                }

                this->apiClient(assetID)
                    ->getBlockFilter(hash)
                    .then([resolve](QByteArray rawData) {
                        resolve(Wire::EncodedBlockFilter::FromJson(
                            QJsonDocument::fromJson(rawData).object().value("filter").toObject()));
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<Wire::TxConfrimation> CachedChainDataSource::getRawTransaction(
    AssetID assetID, QString txid) const
{
    return Promise<Wire::TxConfrimation>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto client = this->apiClient(assetID);

                auto cacheKey = RawTransactionsCache::key_type{ assetID, txid };
                if (auto opt = _rawTranscationsCache.get(cacheKey)) {
                    resolve(*opt);
                    return;
                }

                if (auto tx = _transactionsCache.cacheByIdSync(assetID).transactionByIdSync(txid)) {
                    // can give raw tx
                    if (!tx->serialized().isEmpty()) {
                        Wire::TxConfrimation txconf;
                        txconf.blockHash = tx->blockHash().toStdString();
                        txconf.blockHeight = tx->blockHeight();
                        txconf.txIndex = tx->transactionIndex();
                        txconf.hexTx = tx->serialized().toStdString();
                        resolve(txconf);
                        return;
                    }
                }

                client->getRawTransaction(txid)
                    .then([resolve, cacheKey, this](QByteArray rawTxBytes) {
                        auto rawTx = Wire::TxConfrimation::FromJson(
                            QJsonDocument::fromJson(rawTxBytes).object());
                        _rawTranscationsCache.insert(cacheKey, rawTx);
                        resolve(rawTx);
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<std::string> CachedChainDataSource::getRawTxByIndex(
    AssetID assetID, int64_t blockNum, uint32_t txIndex) const
{
    return Promise<std::string>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto client = this->apiClient(assetID);

                client->getRawTxByIndex(blockNum, txIndex)
                    .then([resolve, this](QByteArray rawTxBytes) {
                        auto txHex = QJsonDocument::fromJson(rawTxBytes)
                                         .object()
                                         .value("hex")
                                         .toString()
                                         .toStdString();

                        resolve(txHex);
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<boost::optional<Wire::TxOut>> CachedChainDataSource::getTxOut(
    AssetID assetID, Wire::OutPoint outpoint) const
{
    return Promise<boost::optional<Wire::TxOut>>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                this->apiClient(assetID)
                    ->getTxOut(QString::fromStdString(outpoint.hash.ToString()), outpoint.index)
                    .then([resolve](QByteArray data) {
                        auto obj = QJsonDocument::fromJson(data).object();
                        resolve(obj.isEmpty() ? boost::none
                                              : boost::make_optional(Wire::TxOut::FromJson(obj)));
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

Promise<QString> CachedChainDataSource::sendRawTransaction(
    AssetID assetID, QString serializedTx) const
{
    return Promise<BlockHash>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            CDataStream ss(bitcoin::ParseHex(serializedTx.toStdString()), bitcoin::SER_NETWORK,
                bitcoin::PROTOCOL_VERSION);
            bitcoin::CMutableTransaction tx;
            ss >> tx;

            LogCDebug(General) << "Sending raw tx" << tx.ToString().c_str();

            this->apiClient(assetID)
                ->sendTransaction(serializedTx)
                .then([resolve](QString txid) { resolve(txid); })
                .fail([reject]() { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

Promise<void> CachedChainDataSource::loadSecondLayerCache(
    AssetID assetID, uint32_t startBlock, Interrupt* onInterrupt) const
{
    auto interrupt = std::make_shared<bool>();
    if (onInterrupt) {
        *onInterrupt = [this, interrupt] {
            QMetaObject::invokeMethod(_executionContext, [interrupt] { *interrupt = true; });
        };
    }
    return Promise<void>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            auto syncHelper = &this->chainSyncHelper(assetID);

            if (*interrupt) {
                reject(QtPromise::QPromiseCanceledException{});
                return;
            }

            this->getBlockHash(assetID, startBlock)
                .then([=](QString startingBlockHash) {
                    auto headersCache = &_secondLayerBlockHeadersCache[assetID];
                    auto hashIndex = &_secondLayerBlockHashIndex[assetID];
                    OnGetBlockHeaderRecursive(resolve, reject, headersCache, hashIndex,
                        startingBlockHash, syncHelper, interrupt.get());
                })
                .fail([reject]() { reject(std::current_exception()); });
        });
    });
}

//==============================================================================

Promise<void> CachedChainDataSource::freeSecondLayerCache(AssetID assetID) const
{
    return Promise<void>([=](const auto& resolve, const auto&) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            _secondLayerBlockHeadersCache.erase(assetID);
            _secondLayerBlockHashIndex.erase(assetID);
            resolve();
        });
    });
}

//==============================================================================

Promise<int64_t> CachedChainDataSource::estimateNetworkFee(AssetID assetID, uint64_t blocks) const
{
    return Promise<int64_t>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            this->apiClient(assetID)
                ->estimateSmartFee(blocks)
                .then([resolve](double networkFeeRate) {
                    resolve(
                        networkFeeRate > 0 ? static_cast<int64_t>(networkFeeRate * COIN) : 1000);
                })
                .fail([reject](const NetworkUtils::ApiErrorException& error) { reject(error); });
        });
    });
}

//==============================================================================

Promise<Wire::TxConfrimation> CachedChainDataSource::getSpendingDetails(
    AssetID assetID, Wire::OutPoint outpoint) const
{
    return Promise<Wire::TxConfrimation>([=](const auto& resolve, const auto& reject) {
        QMetaObject::invokeMethod(_executionContext, [=] {
            try {
                auto client = this->apiClient(assetID);

                client
                    ->getSpendingTx(
                        QString::fromStdString(outpoint.hash.ToString()), outpoint.index)
                    .then([client, resolve, reject, this](QByteArray rawTxBytes) {
                        auto txHash = QJsonDocument::fromJson(rawTxBytes)
                                          .object()
                                          .value("rawTransaction")
                                          .toObject()
                                          .value("txid")
                                          .toString();
                        if (txHash.length() == 0) {
                            reject(std::runtime_error("output_unspent"));
                        }

                        client->getRawTransaction(txHash)
                            .then([resolve, this](QByteArray rawTxBytes) {
                                auto rawTx = Wire::TxConfrimation::FromJson(
                                    QJsonDocument::fromJson(rawTxBytes).object());
                                resolve(rawTx);
                            })
                            .fail([reject]() { reject(std::current_exception()); });
                    })
                    .fail([reject]() { reject(std::current_exception()); });
            } catch (...) {
                reject(std::current_exception());
            }
        });
    });
}

//==============================================================================

static void OnGetBlockHeaderRecursive(const QtPromise::QPromiseResolve<void>& resolve,
    const QtPromise::QPromiseReject<void>& reject,
    std::unordered_map<BlockHash, Wire::VerboseBlockHeader>* headersCache,
    std::unordered_map<size_t, BlockHash>* hashIndex, QString startHash,
    ChainSyncHelper* syncHelper, bool* interruptFlag)
{

    if (*interruptFlag) {
        reject(QtPromise::QPromiseCanceledException{});
        return;
    }

    syncHelper->getLastHeaders(startHash)
        .then([=](std::vector<Wire::VerboseBlockHeader> headers) {
            if (!headers.empty()) {
                // copy headers  into headersCache
                for (auto&& header : std::move(headers)) {
                    headersCache->emplace(QString::fromStdString(header.hash), header);
                    hashIndex->emplace(header.height, QString::fromStdString(header.hash));
                }

                // recursively call this function until we cache all needed headers
                OnGetBlockHeaderRecursive(resolve, reject, headersCache, hashIndex,
                    QString::fromStdString(headers.back().hash), syncHelper, interruptFlag);
            } else {
                resolve();
            }
        })
        .fail(reject);
}

//==============================================================================

ChainSyncHelper& CachedChainDataSource::chainSyncHelper(AssetID assetID) const
{
    if (_chainSyncHelpers.count(assetID) == 0) {
        _chainSyncHelpers[assetID].reset(new ChainSyncHelper(
            apiClient(assetID), _assetsModel.assetById(assetID), _executionContext));
    }

    return *_chainSyncHelpers.at(assetID);
}

//==============================================================================

AbstractBlockExplorerHttpClient* CachedChainDataSource::apiClient(AssetID assetID) const
{
    if (_clients.count(assetID) == 0) {
        _clients[assetID] = _networkingFactory.createBlockExplorerClient(assetID);
    }

    return _clients.at(assetID).get();
}

//==============================================================================

boost::optional<Wire::VerboseBlockHeader> CachedChainDataSource::fetchFromSecondLayerCache(
    AssetID assetID, BlockHash hash) const
{
    if (_secondLayerBlockHeadersCache.count(assetID) > 0
        && _secondLayerBlockHeadersCache.at(assetID).count(hash) > 0) {
        return _secondLayerBlockHeadersCache.at(assetID).at(hash);
    }

    return boost::none;
}

//==============================================================================

boost::optional<Wire::VerboseBlockHeader> CachedChainDataSource::lookupHeaderFromCache(
    AssetID assetID, BlockHash hash) const
{
    auto cacheKey = BlockHeadersCache::key_type{ assetID, hash };
    if (auto opt = _blockHeadersCache.get(cacheKey)) {
        return opt;
    }

    return fetchFromSecondLayerCache(assetID, hash);
}

//==============================================================================
