#include "GRPCServer.hpp"
#include <Chain/AbstractChainDataSource.hpp>
#include <Chain/AbstractChainManager.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/Chain.hpp>
#include <Chain/RegtestChain.hpp>
#include <GRPCTools/ServerUtils.hpp>
#include <LndTools/Protos/Common.pb.h>
#include <LndTools/Protos/KeychainService.grpc.pb.h>
#include <LndTools/Protos/LightWalletService.grpc.pb.h>
#include <Models/AbstractKeychain.hpp>
#include <Models/WalletDataSource.hpp>
#include <Networking/AbstractBlockExplorerHttpClient.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Tools/AppConfig.hpp>
#include <Utils/Logging.hpp>
#include <ZeroMQ/ZMQChainNotifier.hpp>
#include <grpcpp/grpcpp.h>
#include <grpcpp/support/client_interceptor.h>
#include <script/script.h>
#include <streams.h>
#include <utilstrencodings.h>

#include <QFutureWatcher>
#include <QThread>
#include <QtConcurrent>

using namespace grpc;
using namespace lightwalletrpc;

using ChainViewRef = std::shared_ptr<ChainView>;
using ChainViewsCache = std::map<AssetID, ChainViewRef>;
using GetChainView = std::function<ChainView*(AssetID)>;

//==============================================================================

static std::string EncodedBlockHeader(Wire::VerboseBlockHeader::Header data)
{
    CDataStream ssBlock(bitcoin::SER_NETWORK, bitcoin::PROTOCOL_VERSION);
    ssBlock << data.version << bitcoin::uint256S(data.prevBlock)
            << bitcoin::uint256S(data.merkleRoot) << data.timestamp << data.bits << data.nonce;

    return bitcoin::HexStr(ssBlock.begin(), ssBlock.end());
}

//==============================================================================

static AssetID ExtractAssetID(grpc::ServerContext* context)
{
    auto metadata = context->client_metadata();
    auto it = metadata.find("assetid");

    if (it != std::end(metadata)) {
        std::string value(it->second.begin(), it->second.end());
        return std::atoi(value.c_str());
    } else {
        return static_cast<AssetID>(-1);
    }
}

//==============================================================================

struct GRPCServer::ServerImpl : public qgrpc::BaseGrpcServer {

    using UTXOMeta = std::tuple<Wire::OutPoint, Wire::TxOut, size_t>;

    explicit ServerImpl(const WalletAssetsModel& assetsModel, AbstractChainManager& chain,
        AbstractChainDataSource& dataSource, AssetsTransactionsCache& txCache,
        WalletDataSource& walletDataSource, UTXOSetDataSource* utxoDataSource, QObject* parent)
        : qgrpc::BaseGrpcServer("0.0.0.0:" + std::to_string(AppConfig::Instance().config().rpcPort))
        , _assetsModel(assetsModel)
        , _chain(chain)
        , _dataSource(dataSource)
        , _txCache(txCache)
        , _walletDataSource(walletDataSource)
        , _utxoDataSource(utxoDataSource)
        , _parent(parent)
    {
        _keychain = dynamic_cast<AbstractKeychain*>(&_walletDataSource);
    }

    void run() { BaseGrpcServer::run({ &_keychainService, &_lightWalletService }); }

    void initChainNotifiers(std::vector<std::pair<AssetID, int>> configs)
    {
        auto connectHelper = [this](AssetID assetID) {
            auto view = chainView(assetID);
            connect(view, &ChainView::bestBlockHashChanged, _parent,
                [this, assetID](
                    ::BlockHash newTip) { _chainNotifiers.at(assetID)->notifyTip(newTip); });
        };
        for (auto&& cfg : configs) {
            auto notifier = std::make_unique<ZMQChainNotifier>(_dataSource, _parent);
            notifier->SetAddress(QString("tcp://127.0.0.1:%1").arg(cfg.second).toStdString());
            notifier->SetType("pubrawheader");
            if (!notifier->Initialize()) {
                LogCDebug(General) << "Failed to initialize zmq socket";
                continue;
            }

            try {
                connectHelper(cfg.first);
                _chainNotifiers.emplace(cfg.first, notifier.release());
            } catch (std::exception& ex) {
                LogCWarning(General) << "Failed to init chain notifier" << cfg.first << ex.what();
            }
        }
    }

    ChainView* chainView(AssetID assetID)
    {
        if (_chainViews.count(assetID) == 0) {
            _chain
                .getChainView(
                    assetID, AbstractChainManager::ChainViewUpdatePolicy::DecomporessedEvents)
                .then([this](std::shared_ptr<ChainView> chainView) {
                    _chainViews.emplace(chainView->assetID(), chainView);
                })
                .fail([] { LogCDebug(General) << "Failed to get chainView"; })
                .wait();
        }

        return _chainViews.at(assetID).get();
    }

    Promise<std::vector<UTXOMeta>> listUtxosHelper(AssetID assetID, int minConf, int maxConf)
    {
        if (auto view = chainView(assetID)) {
            return view->chainHeight().then([=](size_t height) {
                return _utxoDataSource->listUTXOs(assetID).then(
                    [=](std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>> utxos) {
                        return _txCache.cacheById(assetID).then(
                            [=](AbstractTransactionsCache* cache) {
                                return cache->onChainTransactionsList().then(
                                    [=](OnChainTxList list) {
                                        std::vector<UTXOMeta> result;
                                        for (auto&& it : utxos) {
                                            auto txid = QString::fromStdString(
                                                std::get<0>(it).hash.ToString());
                                            if (auto tx
                                                = TransactionUtils::FindTransaction(list, txid)) {
                                                if (tx->blockHeight() > 0
                                                    && height >= tx->blockHeight()) {
                                                    auto confirmations = height - tx->blockHeight() + 1;
                                                    if (confirmations >= minConf
                                                        && confirmations <= maxConf) {
                                                        result.emplace_back(std::get<0>(it),
                                                            std::get<1>(it), confirmations);
                                                    }
                                                }
                                            }
                                        }
                                        return result;
                                    });
                            });
                    });
            });
        } else {
            return QtPromise::resolve(std::vector<UTXOMeta>());
        }
    }

    void addInterruptHandler(AbstractChainDataSource::Interrupt interrupt)
    {
        std::lock_guard<std::mutex> guard(_interrupsMtx);
        _interrupts.emplace_back(interrupt);
    }

    void removeInterruptHandler(AbstractChainDataSource::Interrupt interrupt)
    {
        // TODO(yuraolex): implement this
        //        std::lock_guard<std::mutex> guard(_interrupsMtx);
        //        _interrupts.erase(std::find(std::begin(_interrupts), std::end(_interrupts),
        //        interrupt));
    }

    void interrupt()
    {
        decltype(_interrupts) interrupts;
        {
            std::lock_guard<std::mutex> guard(_interrupsMtx);
            interrupts.swap(_interrupts);
        }

        for (auto&& interrupt : interrupts) {
            interrupt();
        }
    }

    // BaseGrpcServer interface
protected:
    void registerService() override;

private:
    void registerKeychainService();
    void registerLightWalletService();

private:
    const WalletAssetsModel& _assetsModel;
    AbstractChainManager& _chain;
    AbstractChainDataSource& _dataSource;
    AssetsTransactionsCache& _txCache;
    WalletDataSource& _walletDataSource;
    UTXOSetDataSource* _utxoDataSource;
    AbstractKeychain* _keychain;

    KeychainService::AsyncService _keychainService;
    LightWalletService::AsyncService _lightWalletService;

    ChainViewsCache _chainViews;
    std::map<AssetID, ZMQChainNotifier*> _chainNotifiers;
    QObject* _parent{ nullptr };
    std::vector<AbstractChainDataSource::Interrupt> _interrupts;
    std::mutex _interrupsMtx;
};

//==============================================================================

GRPCServer::GRPCServer(const WalletAssetsModel& assetsModel, AbstractChainManager& chain,
    AbstractChainDataSource& dataSource, AssetsTransactionsCache& txCache,
    WalletDataSource& walletDataSource, UTXOSetDataSource* utxoDataSource, QObject* parent)
    : QObject(parent)
    , _server(new ServerImpl(
          assetsModel, chain, dataSource, txCache, walletDataSource, utxoDataSource, this))
{
}

//==============================================================================

GRPCServer::~GRPCServer()
{
    stop();
}

//==============================================================================

bool GRPCServer::isRunning() const
{
    return _serverThread.isRunning();
}

//==============================================================================

void GRPCServer::start()
{
    _serverThread.rename("Stakenet-GRPLndServerThread");
    _serverThread.start();

    const auto& appConfig = AppConfig::Instance().config();
    const auto& lndConfig = appConfig.lndConfig;

    std::vector<std::pair<AssetID, int>> configs;
    auto push = [&configs, &lndConfig](AssetID id) {
        configs.push_back({ id, lndConfig.daemonConfigById(id).zmqPort });
    };

    push(0);
    push(2);
    push(384);

    _server->initChainNotifiers(configs);

    QMetaObject::invokeMethod(_serverThread.context(), [this, rpcPort = appConfig.rpcPort] {
        LogCCInfo(General) << "LightWalletService running on port" << rpcPort;
        _server->run();
        LogCCInfo(General) << "LightWalletService stopped on port" << rpcPort;
        // ugly workaround for OSX, somewhy it doesn't work
        // if we quit in calling thread then we will wait forever
        _serverThread.quit();
    });
}

//==============================================================================

void GRPCServer::stop()
{
    if (_serverThread.isRunning()) {
        _server->shutdown();
        _serverThread.quit();
        _serverThread.wait();
    }
}

//==============================================================================

void GRPCServer::ServerImpl::registerService()
{
    registerKeychainService();
    registerLightWalletService();
}

//==============================================================================

void GRPCServer::ServerImpl::registerKeychainService()
{
    auto keychainService = &_keychainService;

    registerCall(&KeychainService::AsyncService::RequestDeriveNextKey, keychainService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            _keychain->deriveNextKey(assetID, request->keyfamily())
                .then([sender](::KeyDescriptor desc) {
                    lightwalletrpc::KeyDescriptor response;
                    auto locator = new lightwalletrpc::KeyLocator();
                    locator->set_keyfamily(std::get<0>(desc).first);
                    locator->set_keyindex(std::get<0>(desc).second);
                    response.set_allocated_locator(locator);
                    response.set_pubkey(std::get<1>(desc));
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(
                        grpc::Status(grpc::StatusCode::INTERNAL, "Failed to derive next key"));
                });
        });

    registerCall(&KeychainService::AsyncService::RequestDeriveKey, keychainService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            ::KeyLocator locator;
            locator.first = request->keyfamily();
            locator.second = request->keyindex();

            _keychain->deriveKey(assetID, locator)
                .then([sender](::KeyDescriptor desc) {
                    lightwalletrpc::KeyDescriptor response;
                    auto locator = new lightwalletrpc::KeyLocator();
                    locator->set_keyfamily(std::get<0>(desc).first);
                    locator->set_keyindex(std::get<0>(desc).second);
                    response.set_allocated_locator(locator);
                    response.set_pubkey(std::get<1>(desc));
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(
                        grpc::Status(grpc::StatusCode::INTERNAL, "Failed to derive key"));
                });
        });

    registerCall(&KeychainService::AsyncService::RequestDerivePrivKey, keychainService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            ::KeyLocator locator;
            locator.first = request->locator().keyfamily();
            locator.second = request->locator().keyindex();

            auto descriptor = ::KeyDescriptor{ locator, request->pubkey() };
            _keychain->derivePrivKey(assetID, descriptor)
                .then([sender](std::string privKey) {
                    lightwalletrpc::HexEncoded response;
                    response.set_hash(privKey);
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(
                        grpc::Status(grpc::StatusCode::INTERNAL, "Failed to derive priv key"));
                });
        });

    registerCall(&KeychainService::AsyncService::RequestIsOurAddress, keychainService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto bytes = bitcoin::ParseHex(request->hash());

            _walletDataSource.isOurAddress(assetID, bytes)
                .then([sender](bool isOur) {
                    lightwalletrpc::IsOurAddressResponse response;
                    response.set_isour(isOur);
                    sender->finish(response);
                })
                .fail([sender] {
                    sender->finish(grpc::Status(
                        grpc::StatusCode::INTERNAL, "Failed to determine if our address"));
                });
        });
}

//==============================================================================

void GRPCServer::ServerImpl::registerLightWalletService()
{
    auto lightWalletService = &_lightWalletService;

    registerCall(&LightWalletService::AsyncService::RequestGetChainInfo, lightWalletService,
        [this](auto context, auto request, auto sender) {
            Q_UNUSED(request);
            auto assetID = ExtractAssetID(context);

            if (auto chView = this->chainView(assetID)) {
                QtPromise::all(QVector<Promise<QString>>{ chView->bestBlockHash(),
                                   chView->chainHeight().then(
                                       [](size_t height) { return QString::number(height); }) })
                    .then([sender](const QVector<QString>& info) {
                        lightwalletrpc::GetChainInfoResponse response;

                        response.set_bestblockhash(info.at(0).toStdString());
                        response.set_height(info.at(1).toUInt());
                        sender->finish(response);
                    })
                    .fail([sender](const std::exception& ex) {
                        sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                    })
                    .fail([sender]() {
                        sender->finish(
                            grpc::Status(grpc::UNAVAILABLE, "Failed to execute GetChainInfo"));
                    });
            } else {
                sender->finish(grpc::Status(grpc::UNAVAILABLE, "No chain view"));
            }
        });

    registerCall(&LightWalletService::AsyncService::RequestGetBlockHash, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            _dataSource.getBlockHash(assetID, request->height())
                .then([sender](QString hash) {
                    lightwalletrpc::BlockHash response;
                    response.set_hash(hash.toStdString());
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(grpc::Status(grpc::INTERNAL, "Failed to execute GetBlockHash"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetBlockHeader, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            _dataSource.getBlockHeader(assetID, QString::fromStdString(request->hash()))
                .then([sender](Wire::VerboseBlockHeader header) {
                    lightwalletrpc::HexEncoded response;
                    response.set_hash(EncodedBlockHeader(header.header));
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(
                        grpc::Status(grpc::INTERNAL, "Failed to execute GetBlockHeader"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetBlockHeaderVerbose,
        lightWalletService, [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            _dataSource.getBlockHeader(assetID, QString::fromStdString(request->hash()))
                .then([sender](Wire::VerboseBlockHeader header) {
                    lightwalletrpc::BlockHeader response;
                    response.set_hash(header.hash);
                    response.set_height(header.height);
                    response.set_time(header.header.timestamp);
                    response.set_prevblockhash(header.header.prevBlock);
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(
                        grpc::Status(grpc::INTERNAL, "Failed to execute GetBlockHeaderVerbose"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetBlock, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto hash = QString::fromStdString(request->hash());
            _dataSource.getBlock(assetID, hash)
                .then([sender](std::vector<unsigned char> block) {
                    lightwalletrpc::GetBlockResponse response;
                    response.mutable_block()->append(block.begin(), block.end());
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(grpc::Status(grpc::INTERNAL, "Failed to execute GetBlock"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetFilterBlock, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto hash = QString::fromStdString(request->hash());
            _dataSource.getFilteredBlock(assetID, hash)
                .then([sender](std::vector<std::string> encodedTransactions) {
                    lightwalletrpc::FilterBlockResponse response;
                    for (auto&& tx : encodedTransactions) {
                        response.add_transactions(tx);
                    }
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(
                        grpc::Status(grpc::INTERNAL, "Failed to execute GetFilterBlock"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetBlockFilter, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto hash = QString::fromStdString(request->hash());
            _dataSource.getBlockFilter(assetID, hash)
                .then([sender](Wire::EncodedBlockFilter filter) {
                    lightwalletrpc::BlockFilter response;
                    response.set_n(filter.n);
                    response.set_m(filter.m);
                    response.set_p(filter.p);
                    response.set_bytes(bitcoin::HexStr(filter.bytes));
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(
                        grpc::Status(grpc::INTERNAL, "Failed to execute GetBlockFilter"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetTxOut, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto outpoint = Wire::OutPoint{ bitcoin::uint256S(request->hash()), request->index() };
            _utxoDataSource->getUTXO(assetID, outpoint)
                .then([sender](boost::optional<Wire::TxOut> txOutOpt) {
                    lightwalletrpc::TxOut response;
                    if (txOutOpt) {
                        response.set_scriptpubkey(bitcoin::HexStr(txOutOpt.get().pkScript));
                        response.set_value(txOutOpt.get().value);
                    }
                    sender->finish(response);
                })
                .fail([this, sender, outpoint, assetID](const std::exception&) {
                    _dataSource.getTxOut(assetID, outpoint)
                        .then([sender](boost::optional<Wire::TxOut> txOut) {
                            lightwalletrpc::TxOut response;
                            if (txOut) {
                                response.set_scriptpubkey(bitcoin::HexStr(txOut.get().pkScript));
                                response.set_value(txOut.get().value);
                            }

                            sender->finish(response);
                        })
                        .fail([sender](const std::exception& ex) {
                            sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                        })
                        .fail([sender] {
                            sender->finish(
                                grpc::Status(grpc::StatusCode::INTERNAL, "Failed to get txout"));
                        });
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetSpendingDetails, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto outpoint = Wire::OutPoint{ bitcoin::uint256S(request->hash()), request->index() };
            _dataSource.getSpendingDetails(assetID, outpoint)
                .then([sender](Wire::TxConfrimation rawTx) {
                    GetRawTransactionResponse response;
                    response.set_blockhash(rawTx.blockHash);
                    response.set_transactionhex(rawTx.hexTx);
                    response.set_blockheight(rawTx.blockHeight);
                    response.set_txindex(rawTx.txIndex);
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(
                        grpc::Status(grpc::INTERNAL, "Failed to execute GetSpendingDetails"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetRawTransaction, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            _dataSource.getRawTransaction(assetID, QString::fromStdString(request->txid()))
                .then([sender](Wire::TxConfrimation rawTx) {
                    GetRawTransactionResponse response;
                    response.set_blockhash(rawTx.blockHash);
                    response.set_transactionhex(rawTx.hexTx);
                    response.set_blockheight(rawTx.blockHeight);
                    response.set_txindex(rawTx.txIndex);
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(
                        grpc::Status(grpc::INTERNAL, "Failed to execute GetRawTransaction"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetLastAddress, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            auto type = Enums::AddressType::P2WPKH;
            auto fut = request->ischange() ? _walletDataSource.getChangeAddress(assetID, type)
                                           : _walletDataSource.getReceivingAddress(assetID, type);

            fut.then([sender](QString addr) {
                   GetLastAddressResponse response;
                   response.set_address(addr.toStdString());
                   sender->finish(response);
               })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(
                        grpc::Status(grpc::INTERNAL, "Failed to execute GetLastAddress"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestLockOutpoint, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            auto outpoint = Wire::OutPoint{ bitcoin::uint256S(request->hash()), request->index() };
            _utxoDataSource->lockOutpoint(assetID, outpoint)
                .then([sender]() {
                    Empty response;
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(
                        grpc::Status(grpc::StatusCode::INTERNAL, "Failed to execute LockOutpoint"));
                });
        });
    registerCall(&LightWalletService::AsyncService::RequestUnlockOutpoint, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            auto outpoint = Wire::OutPoint{ bitcoin::uint256S(request->hash()), request->index() };
            _utxoDataSource->unlockOutpoint(assetID, outpoint)
                .then([sender]() {
                    Empty response;
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(
                        grpc::Status(grpc::StatusCode::INTERNAL, "Failed to execute UnlockOutpoint"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestListUtxos, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            this->listUtxosHelper(assetID, request->minconf(), request->maxconf())
                .then([sender](std::vector<UTXOMeta> utxos) {
                    ListUtxoResult response;

                    for (auto&& utxo : utxos) {
                        auto outpoint = std::get<0>(utxo);
                        auto output = std::get<1>(utxo);
                        auto val = response.add_utxos();

                        val->set_vout(outpoint.index);
                        val->set_txid(outpoint.hash.ToString());
                        val->set_value(output.value);
                        val->set_scriptpubkey(bitcoin::HexStr(output.pkScript));
                        val->set_confirmations(std::get<2>(utxo));
                    }

                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(
                        grpc::Status(grpc::StatusCode::INTERNAL, "Failed to execute ListUtxos"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestDumpPrivKey, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto scriptPubKey = bitcoin::ParseHex(request->scriptpubkey());
            _walletDataSource.dumpPrivKey(assetID, scriptPubKey)
                .then([sender](QByteArray serializedKey) {
                    lightwalletrpc::HexEncoded response;
                    response.set_hash(serializedKey.toStdString());
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender]() {
                    sender->finish(grpc::Status(grpc::INTERNAL, "Failed to execute DumpPrivKey"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestSendRawTransaction, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto transactionHex = QString::fromStdString(request->hash());

            _dataSource.sendRawTransaction(assetID, transactionHex)
                .then([sender, this, assetID, request](QString txid) {
                    lightwalletrpc::TxID response;
                    response.set_txid(txid.toStdString());
                    auto tx = TransactionUtils::MutableTxToTransactionRef(
                        MutableTransaction::FromRawTx(assetID, request->hash()), _assetsModel);
                    return _walletDataSource.applyTransactionAsync(tx)
                        .then([this, assetID](OnChainTxRef appliedTx) {
                            return _txCache.cacheById(assetID).then(
                                [appliedTx](AbstractTransactionsCache* cache) {
                                    return cache->addTransactions({ appliedTx });
                                });
                        })
                        .finally([sender, response] { sender->finish(response); });
                })
                .fail([sender](const NetworkUtils::ApiErrorException& error) {
                    if (!error.errors.at(0).message.contains(
                            "The transaction is already in the network")) {
                        sender->finish(grpc::Status(
                            grpc::StatusCode::INTERNAL, error.errors.at(0).message.toStdString()));
                    } else {
                        sender->finish(lightwalletrpc::TxID{}, grpc::Status{});
                    }
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGetRawTxByIndex, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            _dataSource.getRawTxByIndex(assetID, request->blocknum(), request->txindex())
                .then([sender](std::string txHex) {
                    GetRawTxByIndexResponse response;
                    response.set_txhex(txHex);
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(
                        grpc::Status(grpc::StatusCode::INTERNAL, "Failed to get raw tx by index"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestGenerate, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            GenerateResponse response;
            if (auto regtest = dynamic_cast<RegtestDataSource*>(&_dataSource)) {
                auto result = regtest->chain(assetID).generateBlocks(
                    bitcoin::CScript(), request->numblocks());
                for (auto&& hash : result) {
                    response.add_blockhash(hash);
                }
            }

            sender->finish(response);
        });

    registerCall(&LightWalletService::AsyncService::RequestGetConfirmedBalance, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto numConf = request->confs();

            this->listUtxosHelper(assetID, numConf, 99999999)
                .then([sender](std::vector<UTXOMeta> utxos) {
                    GetConfirmedBalanceResponse response;
                    response.set_amount(std::accumulate(std::begin(utxos), std::end(utxos),
                        Balance{ 0 }, [](Balance accum, const auto& utxo) {
                            return accum + std::get<1>(utxo).value;
                        }));

                    sender->finish(response);
                })
                .fail([sender] {
                    sender->finish(
                        grpc::Status(grpc::StatusCode::INTERNAL, "Failed to fetch balance"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestLoadSecondLayerCache, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);

            AbstractChainDataSource::Interrupt interrupt;
            _dataSource.loadSecondLayerCache(assetID, request->startheight(), &interrupt)
                .then([sender]() {
                    LoadCacheResponse response;
                    response.set_loaded(true);
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(grpc::Status(
                        grpc::StatusCode::INTERNAL, "Failed to load second layer cache"));
                })
                .finally([this, interrupt] { this->removeInterruptHandler(interrupt); });

            this->addInterruptHandler(interrupt);
        });

    registerCall(&LightWalletService::AsyncService::RequestFreeSecondLayerCache, lightWalletService,
        [this](auto context, auto /*request*/, auto sender) {
            auto assetID = ExtractAssetID(context);
            _dataSource.freeSecondLayerCache(assetID)
                .then([sender]() {
                    lightwalletrpc::Empty response;
                    sender->finish(response);
                })
                .fail([sender] {
                    sender->finish(grpc::Status(
                        grpc::StatusCode::INTERNAL, "Failed to execute FreeSecondLayerCache"));
                });
        });

    registerCall(&LightWalletService::AsyncService::RequestEstimateNetworkFee, lightWalletService,
        [this](auto context, auto request, auto sender) {
            auto assetID = ExtractAssetID(context);
            auto blocks = assetID == 384 ? 30 : request->blocks();
            _dataSource.estimateNetworkFee(assetID, blocks)
                .then([sender](int64_t networkFee) {
                    EstimateNetworkFeeResponse response;
                    response.set_fee(networkFee);
                    sender->finish(response);
                })
                .fail([sender](const std::exception& ex) {
                    sender->finish(grpc::Status(grpc::StatusCode::INTERNAL, ex.what()));
                })
                .fail([sender] {
                    sender->finish(grpc::Status(
                        grpc::StatusCode::INTERNAL, "Failed to execute EstimateNetworkFee"));
                });
        });
}

//==============================================================================
