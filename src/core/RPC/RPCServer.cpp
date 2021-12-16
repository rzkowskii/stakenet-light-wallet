#if 0

#include "RPCServer.hpp"
#include <Chain/AbstractChainDataSource.hpp>
#include <Chain/AbstractChainManager.hpp>
#include <Chain/Chain.hpp>
#include <Chain/RegtestChain.hpp>
#include <Models/AbstractKeychain.hpp>
#include <Models/WalletDataSource.hpp>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector>
#include <ZeroMQ/ZMQChainNotifier.hpp>
#include <future>
#include <jsonrpccpp/server.h>
#include <jsonrpccpp/server/connectors/httpserver.h>
#include <key_io.h>
#include <script/script.h>
#include <script/standard.h>
#include <serialize.h>
#include <streams.h>
#include <utilstrencodings.h>

//==============================================================================

struct RPCServer::Impl : public QObject, jsonrpc::AbstractServer<RPCServer::Impl>
{
    using ChainViewRef = std::shared_ptr<ChainView>;

    explicit Impl(jsonrpc::AbstractServerConnector &server, AssetID assetID, QObject *parent) :
        QObject(parent),
        jsonrpc::AbstractServer<Impl>(server, jsonrpc::JSONRPC_SERVER_V1),
        _assetID(assetID)
    {
        _rpcThread.start();
    }

    void registerChainNotifier(int zmqPort, AbstractChainDataSource &dataSource);
    void registerChainCalls(AbstractChainManager &chainManager);
    void registerChainViewChanges(AbstractChainManager &chainManager);
    void registerWalletCalls(WalletDataSource &walletDataSource);
    void registerKeychainCalls(AbstractKeychain *keyChain);

    ChainView *chainView(AssetID assetID);

    void rescan(const Json::Value &request, Json::Value &response)
    {
        bool result = false;
        auto blockHash = request[0].asString();

        Json::Value getChainInfoResp;

        getchaininfo({}, getChainInfoResp);

        std::pair<QString, size_t> endBlock  = {
            QString::fromStdString(getChainInfoResp["bestblockhash"].asString()),
            getChainInfoResp["headers"].asInt()
        };

        QMetaObject::invokeMethod(_chainNotifier, [&] {
            result = _chainNotifier->rescan(QString::fromStdString(blockHash), endBlock);
        }, Qt::BlockingQueuedConnection);

        response = result ? std::string("Rescan in progress") : std::string();
    }

    void rescan_abort(const Json::Value &request, Json::Value &response)
    {
        Q_UNUSED(request);
        Q_UNUSED(response);
        QMetaObject::invokeMethod(_chainNotifier, [&] {
            _chainNotifier->abortRescan();
        }, Qt::BlockingQueuedConnection);
    }

    void sendrawtransaction(const Json::Value &request, Json::Value &responce)
    {
        auto transactionHex = QString::fromStdString(request[0].asString());
        Utils::ExecuteJob(_rpcThread, [this, transactionHex, &responce] {
            _dataSource->sendRawTransaction(_assetID, transactionHex).then([&responce](QString txid) {
                responce = txid.toStdString();
            }).wait();
        });
    }

    void getblockhash(const Json::Value &request, Json::Value &response)
    {
        auto blockHeight = request[0].asInt();
        Utils::ExecuteJob(_rpcThread, [this, blockHeight, &response] {
            QString result = QString("No block with height %1 found").arg(blockHeight);
            _dataSource->getBlockHash(_assetID, blockHeight).then([&result](QString hash) {
                result = hash;
            }).wait();
            response = result.toStdString();
        });
    }

    void getchaininfo(const Json::Value &request, Json::Value &response)
    {
        Q_UNUSED(request);

        if(auto chView = chainView(_assetID))
        {
            Utils::ExecuteJob(_rpcThread, [this, &chView, &response] {
                QtPromise::all(QVector<Promise<QString>> {
                                   chView->bestBlockHash(),
                                   chView->chainHeight().then([](size_t height) { return QString::number(height); })
                               }).then([&](const QVector<QString> &info) {
                    response["bestblockhash"] = info.at(0).toStdString();
                    response["headers"] = info.at(1).toUInt();
                }).fail([&]() {
                    response["error"] = "undefined";
                }).wait();
            });
        }
    }

    void getblockheader(const Json::Value &request, Json::Value &response)
    {
        auto hash = QString::fromStdString(request[0].asString());
        bool verbose = true;

        if(request.size() > 1)
        {
            verbose = request[1].asBool();
        }

        Utils::ExecuteJob(_rpcThread, [this, hash, &response, verbose] {
            _dataSource->getBlockHeader(_assetID, hash).then([&response, verbose](Wire::VerboseBlockHeader header) {
                if(verbose)
                {
                    response["hash"] = header.hash;
                    response["height"] = header.height;
                    response["time"] = header.header.timestamp;
                    response["previousblockhash"] = header.header.prevBlock;
                }
                else
                {
                    CDataStream ssBlock(bitcoin::SER_NETWORK, bitcoin::PROTOCOL_VERSION);
                    const auto &data = header.header;
                    ssBlock << data.version << bitcoin::uint256S(data.prevBlock) << bitcoin::uint256S(data.merkleRoot)
                            << data.timestamp << data.bits << data.nonce;
                    response = bitcoin::HexStr(ssBlock.begin(), ssBlock.end());
                }
            }).wait();
        });
    }

    void getfilterblock(const Json::Value &request, Json::Value &response)
    {
        auto hash = QString::fromStdString(request[0].asString());
        Utils::ExecuteJob(_rpcThread, [this, hash, &response] {
            _dataSource->getFilteredBlock(_assetID, hash).then([&response](Wire::MsgFilteredBlock block) {

                for(auto &&tx : block.transactions)
                {
                    response.append(tx);
                }

            }).wait();
        });
    }

    void getrawfilter(const Json::Value &request, Json::Value &response)
    {
        auto hash = QString::fromStdString(request[0].asString());
        Utils::ExecuteJob(_rpcThread, [this, hash, &response] {
            _dataSource->getBlockFilter(_assetID, hash).then([&response](EncodedBlockFilter filter) {
                response["n"] = filter.n;
                response["m"] = static_cast<Json::UInt64>(filter.m);
                response["p"] = filter.p;
                auto bytes = filter.bytes;
                response["bytes"] = bitcoin::HexStr(bytes);
            }).wait();
        });
    }

    void gettxout(const Json::Value &request, Json::Value &response)
    {
        auto hash = bitcoin::uint256S(request[0].asString());
        auto n = static_cast<uint32_t>(request[1].asInt());
        Utils::ExecuteJob(_rpcThread, [this, hash, n, &response] {

            _dataSource->getUTXO(_assetID, { hash, n }).then([&response](boost::optional<Wire::TxOut> txOutOpt) {
                if(txOutOpt)
                {
                    response["scriptpubkeyhex"] = bitcoin::HexStr(txOutOpt.get().pkScript);
                    response["amount"] = static_cast<Json::UInt64>(txOutOpt.get().value);
                }
                else
                {
                    response = "null";
                }
            }).wait();
        });
    }

    void generate(const Json::Value &request, Json::Value &response)
    {
        auto n = static_cast<uint32_t>(request[0].asInt());
        Utils::ExecuteJob(_rpcThread, [this, n, &response] {
            if(auto regtest = dynamic_cast<RegtestDataSource*>(_dataSource))
            {
                auto result = regtest->chain(_assetID).generateBlocks(bitcoin::CScript(), n);
                for(auto &&hash : result)
                {
                    response.append(hash);
                }
            }
        });
    }

    void getbestblockhash(const Json::Value &request, Json::Value &response)
    {
        Q_UNUSED(request);

        if(auto chView = chainView(_assetID))
        {
            Utils::ExecuteJob(_rpcThread, [this, &chView, &response] {
                chView->bestBlockHash().then([&response](QString hash) {
                    response = hash.toStdString();
                }).wait();
            });
        }
    }

    void getrawtransaction(const Json::Value &request, Json::Value &response)
    {
        auto txid = QString::fromStdString(request[0].asString());
        Utils::ExecuteJob(_rpcThread, [this, txid, &response] {
            _dataSource->getRawTransaction(_assetID, txid).then([&response](std::string rawTxEncoded) {
                response = rawTxEncoded;
            }).wait();
        });
    }

    void getlastaddress(const Json::Value &request, Json::Value &response)
    {
        bool isChange = request[0].asBool();
        Utils::ExecuteJob(_rpcThread, [this, isChange, &response] {
            auto id = _assetID;
            auto type = Enums::AddressType::P2WPKH;
            auto fut = isChange ? _walletDataSource->getChangeAddress(id, type) :
                                  _walletDataSource->getReceivingAddress(id, type);
            fut.then([&response](QString addr) {
                response = addr.toStdString();
            }).wait();
        });
    }

    void listutxos(const Json::Value &request, Json::Value &response)
    {
        Q_UNUSED(request);

        Utils::ExecuteJob(_rpcThread, [this, &response] {

            UTXOSetDataSource *utxoDataSource = dynamic_cast<UTXOSetDataSource*>(_walletDataSource);

            if(!utxoDataSource)
            {
                utxoDataSource = dynamic_cast<UTXOSetDataSource*>(_dataSource);
            }

            if(utxoDataSource)
            {
                utxoDataSource->listUTXOs(_assetID).then([&response](std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>> utxos) {

                    for(auto &&utxo : utxos)
                    {
                        auto outpoint = std::get<0>(utxo);
                        auto output = std::get<1>(utxo);

                        Json::Value val;

                        val["vout"] = static_cast<Json::UInt>(outpoint.index);
                        val["txid"] = outpoint.hash.ToString();
                        val["amount"] = static_cast<Json::Int64>(output.value);
                        val["scriptPubKey"] = bitcoin::HexStr(output.pkScript);
                        val["confirmations"] = static_cast<Json::Int64>(6);

                        response.append(val);
                    }

                }).wait();
            }
        });
    }

    void dumpprivkey(const Json::Value &request, Json::Value &response)
    {
        auto scriptPubKeyHex = request[0].asString();
        Utils::ExecuteJob(_rpcThread, [this, &response, scriptPubKeyHex] {
            _walletDataSource->dumpPrivKey(_assetID, bitcoin::ParseHex(scriptPubKeyHex)).then([&response](QByteArray serializedKey) {
                response = serializedKey.toStdString();
            }).wait();
        });
    }

    void derivekey(const Json::Value &request, Json::Value &response)
    {
        KeyLocator locator;
        locator.first = request[0].asUInt();
        locator.second = request[1].asUInt();

        Utils::ExecuteJob(_rpcThread, [this, &response, locator] {
            _keychain->deriveKey(_assetID, locator).then([&response](KeyDescriptor desc) {
                Json::Value locator;
                locator["family"] = std::get<0>(desc).first;
                locator["index"] = std::get<0>(desc).second;
                response["locator"] = locator;
                response["pubKey"] = std::get<1>(desc);
            }).wait();
        });
    }

    void derivenextkey(const Json::Value &request, Json::Value &response)
    {
        auto family = request[0].asUInt();

        Utils::ExecuteJob(_rpcThread, [this, &response, family] {
            _keychain->deriveNextKey(_assetID, family).then([&response](KeyDescriptor desc) {
                Json::Value locator;
                locator["family"] = std::get<0>(desc).first;
                locator["index"] = std::get<0>(desc).second;
                response["locator"] = locator;
                response["pubKey"] = std::get<1>(desc);
            }).wait();
        });
    }

    void deriveprivkey(const Json::Value &request, Json::Value &response)
    {
        KeyLocator locator;
        locator.first = request[0]["family"].asUInt();
        locator.second = request[0]["index"].asUInt();
        std::string encodedPubKey = request[1].asString();
        KeyDescriptor descriptor = KeyDescriptor { locator, encodedPubKey };

        Utils::ExecuteJob(_rpcThread, [this, &response, descriptor] {
            _keychain->derivePrivKey(_assetID, descriptor).then([&response](std::string encodedPrivKey) {
                response = encodedPrivKey;
            }).wait();
        });
    }

    AbstractChainDataSource *_dataSource { nullptr };
    ZMQChainNotifier *_chainNotifier { nullptr };
    AbstractChainManager *_chainManager { nullptr };
    WalletDataSource *_walletDataSource { nullptr };
    AbstractKeychain *_keychain { nullptr };
    std::map<AssetID, ChainViewRef> _chainViews;
    Utils::WorkerThread _rpcThread;
    AssetID _assetID;
};

//==============================================================================

RPCServer::RPCServer(QObject *parent) : QObject(parent)
{
    auto addBackend = [this](AssetID assetID, int rpcPort, int zmqPort) {
        ConnectorPtr connector(new jsonrpc::HttpServer(rpcPort, {}, {}, 1));
        ImplPtr impl(new RPCServer::Impl(*connector, assetID, this));

        _rpcBackends.emplace_back(Backend {std::move(impl), std::move(connector), rpcPort, zmqPort, assetID});
    };

    addBackend(0, 12345, 23456);
    addBackend(2, 12346, 23457);
    addBackend(384, 12347, 23458);
}

//==============================================================================

RPCServer::~RPCServer()
{
    stop();
}

//==============================================================================

void RPCServer::start()
{
    try
    {
        for(auto &&backend : _rpcBackends)
        {
            backend.impl->StartListening();
        }
    }
    catch (jsonrpc::JsonRpcException &e)
    {
        LogCDebug(General) << "Exception starting listening rpc port" << e.what();
        throw;
    }
}

//==============================================================================

void RPCServer::stop()
{
    for(auto &&backend : _rpcBackends)
    {
        backend.impl->StopListening();
    }
}

//==============================================================================

void RPCServer::registerChainCalls(AbstractChainManager &chainManager)
{
    for(auto &&backend : _rpcBackends)
    {
        Q_ASSERT(backend.impl->_chainNotifier);
        backend.impl->registerChainCalls(chainManager);
    }
}

//==============================================================================

void RPCServer::registerChainNotifier(AbstractChainDataSource &dataSource)
{
    for(auto &&backend : _rpcBackends)
    {
        backend.impl->_chainNotifier = new ZMQChainNotifier(dataSource, this);
        backend.impl->registerChainNotifier(backend.zmqPort, dataSource);
    }
}

//==============================================================================

void RPCServer::registerWalletCalls(WalletDataSource &walletDataSource)
{
    for(auto &&backend : _rpcBackends)
    {
        backend.impl->registerWalletCalls(walletDataSource);
    }
}

//==============================================================================

void RPCServer::Impl::registerChainNotifier(int zmqPort, AbstractChainDataSource &dataSource)
{
    _dataSource = &dataSource;
    _chainNotifier->SetAddress(QString("tcp://127.0.0.1:%1").arg(zmqPort).toStdString());
    _chainNotifier->SetType("pubrawheader");
    if(!_chainNotifier->Initialize())
    {
        LogCDebug(General) << "Failed to initialize zmq socket";
    }

    using namespace jsonrpc;

    // bindAndAddMethod( "methodName", PARAMS_TYPE, RETURN_TYPE, "input_value_description", INPUT_TYPE, NULL -> end of va_list)
    bindAndAddMethod(Procedure("getblockhash", PARAMS_BY_POSITION, JSON_STRING, "block_height", JSON_INTEGER, NULL),
                     &RPCServer::Impl::getblockhash);

    bindAndAddMethod(Procedure("rescanstart", PARAMS_BY_POSITION, JSON_STRING, "starting_block_hash", JSON_STRING, NULL),
                     &RPCServer::Impl::rescan);

    bindAndAddMethod(Procedure("rescanabort", PARAMS_BY_POSITION, JSON_STRING, NULL),
                     &RPCServer::Impl::rescan_abort);

    bindAndAddMethod(Procedure("getblockheader", PARAMS_BY_POSITION, JSON_OBJECT, "hash", JSON_STRING, "verbose", JSON_BOOLEAN, NULL),
                     &RPCServer::Impl::getblockheader);

    bindAndAddMethod(Procedure("getfilterblock", PARAMS_BY_POSITION, JSON_ARRAY, "hash", JSON_STRING, NULL),
                     &RPCServer::Impl::getfilterblock);

    bindAndAddMethod(Procedure("sendrawtransaction", PARAMS_BY_POSITION, JSON_OBJECT, "hexTransaction", JSON_STRING, "allowHighFee", JSON_BOOLEAN, NULL),
                     &RPCServer::Impl::sendrawtransaction);

    bindAndAddMethod(Procedure("getrawfilter", PARAMS_BY_POSITION, JSON_OBJECT, "hash", JSON_STRING, NULL),
                     &RPCServer::Impl::getrawfilter);

    bindAndAddMethod(Procedure("getunspentoutput", PARAMS_BY_POSITION, JSON_OBJECT, "hash", JSON_STRING, "index", JSON_INTEGER, NULL),
                     &RPCServer::Impl::gettxout);

    bindAndAddMethod(Procedure("getrawtransaction", PARAMS_BY_POSITION, JSON_STRING, "txid", JSON_STRING, "verbose", JSON_INTEGER, NULL),
                     &RPCServer::Impl::getrawtransaction);
}

//==============================================================================

void RPCServer::Impl::registerChainCalls(AbstractChainManager &chainManager)
{
    _chainManager = &chainManager;

    using namespace jsonrpc;

    bindAndAddMethod(Procedure("getbestblockhash", PARAMS_BY_POSITION, JSON_STRING, NULL),
                     &RPCServer::Impl::getbestblockhash);

    bindAndAddMethod(Procedure("getchaininfo", PARAMS_BY_POSITION, JSON_OBJECT, NULL),
                     &RPCServer::Impl::getchaininfo);

    bindAndAddMethod(Procedure("generate", PARAMS_BY_POSITION, JSON_OBJECT, "n", JSON_INTEGER, NULL),
                     &RPCServer::Impl::generate);


    auto connectHelper = [this](AssetID assetID) {
        auto view = chainView(assetID);
        connect(view, &ChainView::bestBlockHashChanged, this, [this, view](BlockHash newTip) {
            _chainNotifier->notifyTip(newTip);
        });
    };

    connectHelper(_assetID);
}

//==============================================================================

void RPCServer::Impl::registerWalletCalls(WalletDataSource &walletDataSource)
{
    _walletDataSource = &walletDataSource;

    registerKeychainCalls(dynamic_cast<AbstractKeychain*>(_walletDataSource));

    using namespace jsonrpc;

    bindAndAddMethod(Procedure("getlastaddress", PARAMS_BY_POSITION, JSON_STRING, "is_change", JSON_BOOLEAN, NULL),
                     &RPCServer::Impl::getlastaddress);

    bindAndAddMethod(Procedure("listutxos", PARAMS_BY_POSITION, JSON_STRING, NULL),
                     &RPCServer::Impl::listutxos);

    bindAndAddMethod(Procedure("dumpprivkey", PARAMS_BY_POSITION, JSON_STRING, "scriptPubKeyHex", JSON_STRING, NULL),
                     &RPCServer::Impl::dumpprivkey);
}

//==============================================================================

void RPCServer::Impl::registerKeychainCalls(AbstractKeychain *keyChain)
{
    _keychain = keyChain;
    Q_ASSERT(_keychain);

    using namespace jsonrpc;

    bindAndAddMethod(Procedure("derivekey", PARAMS_BY_POSITION, JSON_OBJECT, "family", JSON_INTEGER, "index", JSON_INTEGER, NULL),
                     &RPCServer::Impl::derivekey);

    bindAndAddMethod(Procedure("derivenextkey", PARAMS_BY_POSITION, JSON_OBJECT, "family", JSON_INTEGER, NULL),
                     &RPCServer::Impl::derivenextkey);

    bindAndAddMethod(Procedure("deriveprivkey", PARAMS_BY_POSITION, JSON_STRING, "locator", JSON_OBJECT, "pubkey", JSON_STRING, NULL),
                     &RPCServer::Impl::deriveprivkey);
}

//==============================================================================

ChainView *RPCServer::Impl::chainView(AssetID assetID)
{
    if(_chainViews.count(assetID) == 0)
    {
        _chainManager->getChainView(assetID).then([this](std::shared_ptr<ChainView> chainView) {
            _chainViews.emplace(chainView->assetID(), chainView);
        }).fail([] {
            LogCDebug(General) << "Failed to get chainView";
        }).wait();
    }

    return _chainViews.at(assetID).get();
}

//==============================================================================

#endif
