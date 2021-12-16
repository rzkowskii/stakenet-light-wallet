#ifndef TST_PORTALHTTPCLIENT_HPP
#define TST_PORTALHTTPCLIENT_HPP

#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPointer>
#include <QSignalSpy>
#include <QTimer>
#include <gtest/gtest.h>
#include <memory>

#include <Chain/AbstractAssetAccount.hpp>
#include <Chain/AbstractTransactionsCache.hpp>
#include <Chain/Chain.hpp>
#include <Chain/ChainManager.hpp>
#include <Chain/ChainSyncManager.hpp>
#include <Chain/RegtestChain.hpp>
#include <Data/Wallet.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Factories/ApiClientNetworkingFactory.hpp>
#include <Factories/ChainSyncManagerFactory.hpp>
#include <Models/SyncService.hpp>
#include <Networking/AbstractBlockExplorerHttpClient.hpp>
#include <Networking/NetworkConnectionState.hpp>
#include <Networking/RequestHandlerImpl.hpp>
#include <Networking/XSNBlockExplorerHttpClient.hpp>
#include <golomb/gcs.h>
#include <utilstrencodings.h>
#include <walletdb.h>

using namespace testing;
namespace SyncServiceImpl {
class ImInitialSyncProcessor;
}

class MockDataSource : public WalletDataSource {
public:
    MockDataSource(bool isLoaded = false, bool isCreated = true)
        : WalletDataSource()
        , _isCreated(isCreated)
        , _isLoaded(isLoaded)
    {
    }

    using AddressMap = std::map<AssetID, std::set<Address>>;
    using TransactionsMap = std::map<AssetID, TransactionsList>;
    using AddressBook = std::map<AssetID, AddressesList>;

    void setAddressMap(AddressMap addressMap) { _addresses = addressMap; }
    void addAddress(AssetID assetID, Address address)
    {
        auto addressEntry = _addressBook.find(assetID);
        if (addressEntry != _addressBook.end()) {
            auto addressesById = addressEntry->second;

            if (std::find(addressesById.begin(), addressesById.end(), address)
                == addressesById.end()) {
                _addressBook.at(assetID).push_back(address);
            }
        } else {
            _addressBook.emplace(assetID, AddressesList{ address });
        }
    }

    Promise<QString> getReceivingAddress(AssetID id, Enums::AddressType addressType) const override
    {
        //        if(_addresses.count(id) == 0)
        //        {
        //            return QString();
        //        }

        //        auto addressesEntry = _addresses.at(id);
        //        auto chainIndex = addressesEntry.second;
        //        return /*chainIndex < addressesEntry.first.size() ?
        //        addressesEntry.first.at(chainIndex) : */QString();

        return Promise<QString>(
            [this](const auto& resolve, const auto& reject) { reject(QString()); });
    }

    Promise<QString> getChangeAddress(AssetID id, Enums::AddressType addressType) const override
    {
        return Promise<QString>(
            [this](const auto& resolve, const auto& reject) { reject(QString()); });
    }

    void markAddressAsUsed(AssetID id, QString address)
    {
        //        auto &addressesEntry = _addresses.at(id);
        //        auto &chainIndex = addressesEntry.second;
        //        if(chainIndex < addressesEntry.first.size() &&
        //                addressesEntry.first[chainIndex] == address)
        //        {
        //            ++chainIndex;
        //        }
    }
    Promise<QString> getMnemonic() const override
    {
        return Promise<QString>(
            [this](const auto& resolve, const auto& reject) { reject(QString()); });
    }

    Promise<void> encryptWallet(SecureString password)
    {
        return Promise<void>::reject(std::runtime_error("not implemented"));
    }

    bool isLoaded() const override { return _isLoaded; }
    bool isCreated() const override { return _isCreated; }
    bool isEmpty(AssetID assetID) const override { return true; }
    QString identityPubKey() const override { return { "mocked-test-pub-key" }; }

    OnChainTxRef applyTransaction(OnChainTxRef source, LookupTxById lookupTx) override
    {
        OnChainTx::Inputs inputs;
        OnChainTx::Outputs outputs;
        auto assetId = source->assetID();
        OnChainTxRef result;
        if (_addresses.count(assetId) > 0) {
            const auto& addresses = _addresses.at(assetId);
            std::copy_if(std::begin(source->outputs()), std::end(source->outputs()),
                std::back_inserter(outputs), [&addresses](const auto& output) {
                    //                return addresses.count(output.address()) > 0;
                    return false;
                });
        }

        if (!outputs.empty()) {
            result = std::make_shared<OnChainTx>(source->assetID(), source->txId(),
                source->blockHash(), source->blockHeight(), source->transactionIndex(),
                source->transactionDate(), inputs, outputs, source->type(), TxMemo{});
        }

        return result;
    }

    Promise<OnChainTxRef> applyTransactionAsync(OnChainTxRef source) override
    {
        return QtPromise::resolve(source);
    }

    void undoTransaction(OnChainTxRef transaction) override {}

    Promise<void> createWalletWithMnemonic() override
    {
        return Promise<void>([this](const auto& resolve, const auto&) {
            _isCreated = true;
            _isLoaded = true;
            resolve();
        });
    }
    Promise<void> loadWallet(std::optional<SecureString> password) override
    {
        return Promise<void>([this](const auto& resolve, const auto&) {
            _isLoaded = true;
            resolve();
        });
    }
    Promise<MutableTransactionRef> createSendTransaction(bitcoin::UTXOSendTxParams params) override
    {
        Q_UNUSED(params);
        return Promise<MutableTransactionRef>([](const auto& resolve, const auto& reject) {
            Q_UNUSED(resolve);
            reject();
        });
    }
    Promise<void> restoreWalletWithMnemAndPass(QString mnemonic) override
    {
        return Promise<void>([this](const auto& resolve, const auto& reject) { reject(); });
    }
    Promise<AddressesList> getAllKnownAddressesById(AssetID assetID) const override
    {
        return Promise<AddressesList>([this, assetID](const auto& resolve, const auto&) {
            resolve(_addressBook.count(assetID) > 0 ? _addressBook.at(assetID) : AddressesList());
        });
    }
    Promise<Enums::AddressType> getAddressType(AssetID assetID) const
    {
        return QtPromise::resolve(_addressType);
    }
    Enums::AddressType getAddressTypeSync(AssetID assetID) const override { return _addressType; }

    Promise<void> setAddressType(AssetID assetID, Enums::AddressType addressType)
    {
        return Promise<void>([addressType, this](const auto& resolver, const auto&) {
            _addressType = addressType;
            resolver();
        });
    }

    Promise<bool> isOurAddress(
        AssetID assetID, std::vector<unsigned char> scriptPubKey) const override
    {
        return QtPromise::resolve(false);
    }

    bool isEncrypted() const override { return true; }

private:
    AddressMap _addresses;
    TransactionsMap _transactions;
    AddressBook _addressBook;
    TransactionsList _emptyList;
    Enums::AddressType _addressType;

    bool _isCreated = false;
    bool _isLoaded = false;
    bool _isRestored = false;
};

class MockTransactionsCache : public AbstractTransactionsCache {
    // AbstractTransactionsCache interface
public:
    MockTransactionsCache()
        : AbstractTransactionsCache()
    {
    }

    //    Promise<void> load(bool wipe) override
    //    {
    //        return Promise<void>([this](const auto& resolve, const auto&) {
    //            resolve();
    //        });
    //    }

    //    Promise<TransactionsMap> transactionsMap() const override
    //    {
    //        return Promise<TransactionsMap>([this](const auto& resolve, const auto&) {
    //            resolve(_transactionsMap);
    //        });
    //    }
    //    Promise<TransactionsList> transactionsListByID(AssetID assetId) const override
    //    {
    //        return Promise<TransactionsList>([this, assetId](const auto& resolve, const auto&) {
    //            resolve(_transactionsMap.count(assetId) > 0 ? _transactionsMap.at(assetId) :
    //            TransactionsList());
    //        });
    //    }
    //    Promise<TransactionRef> transactionById(AssetID assetId, QString txId) const override
    //    {
    //        return Promise<TransactionRef>([this, assetId, txId](const auto& resolve, const auto&)
    //        {
    //            return this->transactionsListByID(assetId).then([=](TransactionsList list) {
    //                resolve(TransactionUtils::FindTransaction(list, txId));
    //            });
    //        });
    //    }
    //    TransactionRef transactionByIdSync(AssetID assetID, QString txId) const override
    //    {
    //        auto list = _transactionsMap.count(assetID) > 0 ? _transactionsMap.at(assetID) :
    //        TransactionsList();

    //        return TransactionUtils::FindTransaction(list, txId);
    //    }

    //    Promise<void> addTransactions(TransactionRef transaction) override
    //    {
    //        return Promise<void>([this, transaction](const auto& resolve, const auto&) {
    //            this->addTransactionsSync(transaction);
    //            resolve();
    //        });
    //    }

    //    TransactionsList onChainTransactionsListSync(AssetID assetId) const override
    //    {
    //        return _transactionsMap.count(assetId) > 0 ? _transactionsMap.at(assetId) :
    //        TransactionsList();
    //    }

    //    Promise<std::vector<QString>> transactionsInBlock(AssetID assetID, BlockHash blockHash)
    //    const override
    //    {
    //        return QtPromise::resolve(transactionsInBlockSync(assetID, blockHash));
    //    }

    //    std::vector<QString> transactionsInBlockSync(AssetID assetID, BlockHash blockHash) const
    //    override
    //    {
    //        return {};
    //    }

    //    bool isCreated() const override
    //    {
    //        return true;
    //    }

    //    void addTransactionsSync(TransactionRef transaction) override
    //    {
    //        if (transaction) {
    //            _transactionsMap[transaction->assetID()].push_back(transaction);
    //        }
    //    }

    // AbstractTransactionsCache interface
public:
    Promise<TransactionsList> transactionsList() const override { return Promise<TransactionsList>::resolve({}); }
    Promise<void> addTransactions(std::vector<Transaction> transaction) override { return Promise<void>::resolve(); }
    void addTransactionsSync(std::vector<Transaction> transaction) override {}
    Promise<OnChainTxList> onChainTransactionsList() const override { return Promise<OnChainTxList>::resolve({}); }
    Promise<OnChainTxRef> transactionById(QString txId) const override { return Promise<OnChainTxRef>::resolve({});}
    Promise<std::vector<QString>> transactionsInBlock(BlockHash blockHash) const override { return Promise<std::vector<QString>>::resolve({});}
    OnChainTxRef transactionByIdSync(QString txId) const override { return {}; }
    const OnChainTxList& onChainTransactionsListSync() const override { return {}; }
    std::vector<QString> transactionsInBlockSync(BlockHash blockHash) const override { return {};}
    Promise<LightningPaymentList> lnPaymentsList() const override { return Promise<LightningPaymentList>::resolve({});}
    Promise<LightningInvoiceList> lnInvoicesList() const override { return Promise<LightningInvoiceList>::resolve({});}
    const LightningPaymentList& lnPaymentsListSync() const override { return {}; }
    const LightningInvoiceList& lnInvoicesListSync() const override { return {}; }
    Promise<EthOnChainTxList> onEthChainTransactionsList() const override { return Promise<EthOnChainTxList>::resolve({}); }
    Promise<EthOnChainTxRef> ethTransactionById(QString txId) const override { return Promise<EthOnChainTxRef>::resolve({}); }
    EthOnChainTxRef ethTransactionByIdSync(QString txId) const override { return {}; }
    const EthOnChainTxList& ethOnChainTransactionsListSync() const override { return {}; }

    OnChainTxMap _transactionsMap;
};

struct MockAssetsTransactionsCache : public AssetsTransactionsCache {

    // AssetsTransactionsCache interface
public:
    MockAssetsTransactionsCache() { _caches.emplace(0, std::make_unique<MockTransactionsCache>()); }
    Promise<void> load(bool wipe) override { return Promise<void>::resolve(); }
    bool isCreated() const override { return false; }
    AbstractTransactionsCache& cacheByIdSync(AssetID assetId) override
    {
        return *_caches.at(assetId);
    }
    std::vector<AssetID> availableCaches() const override { return { 0 }; }

    std::unordered_map<AssetID, std::unique_ptr<MockTransactionsCache>> _caches;
};

struct MockFilterMatcher : public BlockFilterMatcher {
    MockFilterMatcher(AssetID assetID, std::set<QString> matches)
        : BlockFilterMatcher(assetID)
        , _matches(matches)
    {
    }

    bool match(const BlockIndex& blockIndex) const override
    {
        return _matches.count(blockIndex.hash()) > 0;
    }

    std::set<QString> _matches;
};

struct MockFilterMatchable : public BlockFilterMatchable {

    BFMatcherUniqueRef createMatcher(AssetID assetID) const override
    {
        std::set<QString> matches;
        if (_matches.count(assetID) > 0) {
            matches = _matches.at(assetID);
        }

        return BFMatcherUniqueRef(new MockFilterMatcher(assetID, matches));
    }

    std::map<AssetID, std::set<QString>> _matches;
};

class MockChainManager : public AbstractChainManager {

public:
    MockChainManager(const WalletAssetsModel& assetsModel)
        : AbstractChainManager(assetsModel)
    {
    }

    Promise<void> loadChains(std::vector<AssetID> chains) override
    {
        return Promise<void>([chains, this](const auto& resolve, const auto&) {
            auto get
                = [](size_t) -> boost::optional<Wire::VerboseBlockHeader> { return boost::none; };

            for (auto&& id : chains) {
                _chains.emplace(id,
                    std::unique_ptr<Chain>(new Chain(id, [](Wire::VerboseBlockHeader) {}, get)));
            }

            resolve();
        });
    }

    Promise<void> loadFromBootstrap(QString bootstrapFile) override
    {
        return Promise<void>([](const auto& resolve, const auto& reject) { reject(); });
    }
};

class MockedChainDataSource : public AbstractChainDataSource {
    // AbstractChainDataSource interface
public:
    Promise<std::vector<Wire::VerboseBlockHeader>> getBlockHeaders(
        AssetID assetID, BlockHash startingBlockHash) const override
    {
        return Promise<std::vector<Wire::VerboseBlockHeader>>::reject(
            QtPromise::QPromiseUndefinedException{});
    }
    Promise<Wire::VerboseBlockHeader> getBlockHeader(AssetID assetID, BlockHash hash) const override
    {
        return Promise<Wire::VerboseBlockHeader>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<BlockHash> getBlockHash(AssetID assetID, size_t blockHeight) const override
    {
        return Promise<BlockHash>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<std::vector<unsigned char>> getBlock(AssetID assetID, BlockHash hash) const override
    {
        return Promise<std::vector<unsigned char>>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<Wire::StrippedBlock> getLightWalletBlock(
        AssetID assetID, BlockHash hash, int64_t blockHeight) const override
    {
        return Promise<Wire::StrippedBlock>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<std::tuple<BlockHash, BlockHeight>> getBestBlockHash(AssetID assetID) const override
    {
        return Promise<std::tuple<BlockHash, BlockHeight>>::reject(
            QtPromise::QPromiseUndefinedException{});
    }
    Promise<std::vector<std::string>> getFilteredBlock(
        AssetID assetID, BlockHash hash) const override
    {
        return Promise<std::vector<std::string>>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<Wire::EncodedBlockFilter> getBlockFilter(AssetID assetID, BlockHash hash) const override
    {
        return Promise<Wire::EncodedBlockFilter>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<Wire::TxConfrimation> getRawTransaction(AssetID assetID, QString txid) const override
    {
        return Promise<Wire::TxConfrimation>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<std::string> getRawTxByIndex(
        AssetID assetID, int64_t blockNum, uint32_t txIndex) const override
    {
        return Promise<std::string>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<boost::optional<Wire::TxOut>> getTxOut(AssetID, Wire::OutPoint outpoint) const override
    {
        return Promise<boost::optional<Wire::TxOut>>::reject(
            QtPromise::QPromiseUndefinedException{});
    }
    Promise<QString> sendRawTransaction(AssetID assetID, QString serializedTx) const override
    {
        return Promise<QString>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<void> loadSecondLayerCache(
        AssetID assetID, uint32_t startBlock, Interrupt* onInterrupt) const override
    {
        return Promise<void>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<void> freeSecondLayerCache(AssetID assetID) const override
    {
        return Promise<void>::reject(QtPromise::QPromiseUndefinedException{});
    }
    Promise<int64_t> estimateNetworkFee(AssetID assetID, uint64_t blocks) const override
    {
        return Promise<int64_t>::reject(QtPromise::QPromiseUndefinedException{});
    }
};

class MockedAssetsAccountsCache : public AssetsAccountsCache {

    // AssetsAccountsCache interface
public:
    Promise<void> load() override { return Promise<void>::resolve(); }
    AbstractAssetAccount& cacheByIdSync(AssetID assetId) override
    {
        throw std::runtime_error("not implemented");
    }
    AbstractMutableAccount& mutableCacheByIdSync(AssetID assetId) override
    {
        throw std::runtime_error("not implemented");
    }
    std::vector<AssetID> availableCaches() const override { return {}; }
};

//==============================================================================

class MockedAccountDataSource : public AccountDataSource {

    // AccountDataSource interface
public:
    Promise<QString> getAccountAddress(AssetID id) const override
    {
        return Promise<QString>::reject(std::runtime_error("not implemented"));
    }
    Promise<eth::SignedTransaction> createSendTransaction(eth::AccountSendTxParams params) override
    {
        Q_UNUSED(params);
        return Promise<eth::SignedTransaction>([](const auto& resolve, const auto& reject) {
            Q_UNUSED(resolve);
            reject();
        });
    }

    Promise<QString> dumpPrivKey(AssetID id) const override
    {
        return Promise<QString>::resolve({});
    }
};

//==============================================================================

class WalletTests : public ::testing::Test {
protected:
    ApiClientNetworkingFactory _apiClientsFactory;
    WalletAssetsModel assetsModel;
    CoinAsset _xsnAsset;
    CoinAsset _ltcAsset;
    std::unique_ptr<ChainSyncManagerFactory> _smFactory;
    std::unique_ptr<AbstractNetworkingFactory> _ntFactory;
    std::unique_ptr<Utils::WorkerThread> _worker;
    std::unique_ptr<MockDataSource> _mockDataSource;
    std::unique_ptr<MockAssetsTransactionsCache> _cache;
    std::unique_ptr<MockFilterMatchable> _filterMatchable;
    std::unique_ptr<MockedChainDataSource> _chainDataSource;
    std::unique_ptr<MockChainManager> _chainManager;
    std::unique_ptr<MockedAssetsAccountsCache> _assetsAccountsCache;
    std::unique_ptr<MockedAccountDataSource> _accountDataSource;

    WalletTests()
        : assetsModel("assets_conf.json")
        , _xsnAsset(assetsModel.assetById(384))
        , // XSN coin id
        _ltcAsset(assetsModel.assetById(2)) // LTC coin id
    {
    }

    void SetUp() override
    {
        NetworkConnectionState::Initialize();
        _worker.reset(new Utils::WorkerThread);
        _worker->start();
        _filterMatchable.reset(new MockFilterMatchable);
        _mockDataSource.reset(new MockDataSource);
        _cache.reset(new MockAssetsTransactionsCache);
        _chainDataSource.reset(new MockedChainDataSource);
        _assetsAccountsCache.reset(new MockedAssetsAccountsCache);
        _accountDataSource.reset(new MockedAccountDataSource);
        _smFactory.reset(new ChainSyncManagerFactory(&assetsModel, &_apiClientsFactory, *_worker,
            *_mockDataSource, *_accountDataSource, *_chainDataSource, *_cache,
            *_assetsAccountsCache, *_filterMatchable));
        _ntFactory.reset(new ApiClientNetworkingFactory);
        _chainManager.reset(new MockChainManager(assetsModel));
    }

    void TearDown() override
    {
        NetworkConnectionState::Shutdown();
        _worker->quit();
        _worker->wait();
        _worker.reset();
        _filterMatchable.reset();
        _mockDataSource.reset();
        _cache.reset();
        _smFactory.reset();
        _ntFactory.reset();
        _chainManager.reset();
    }
};

#if 0

TEST(CoreTests, getTransactionsForAddress)
{
    std::unique_ptr<RequestHandlerImpl> requestHandler(new RequestHandlerImpl(new QNetworkAccessManager(), "https://xsnexplorer.io/api/"));
    XSNBlockExplorerHttpClient XSNBlockExplorerApi(std::move(requestHandler));

#if 0
    QSignalSpy spy(&XSNBlockExplorerApi, &XSNBlockExplorerHttpClient::getTransactionsForAddressFinished);
    const QString address = "XoYwNriUZRhKuLKtpwDp87ScmxkM1HgcJs";
    XSNBlockExplorerApi.getTransactionsForAddress(address, 2, "asc", QString());

    spy.wait(12000);
    ASSERT_EQ(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QJsonDocument itemDoc = QJsonDocument::fromJson(arguments.at(2).toByteArray());
    QJsonObject rootObject = itemDoc.object();
    QJsonArray data = rootObject.value("data").toArray();
    QJsonObject firstTransaction = data[0].toObject();
    ASSERT_EQ(firstTransaction.value("id").toString(), "ded30d1e51365d297fd235cebd452f324a138b9916b65c7e5921839a4d26aeff");
#endif
}

TEST(CoreTests, sendTransactionsByAddress)
{
    std::unique_ptr<RequestHandlerImpl> requestHandler(new RequestHandlerImpl(new QNetworkAccessManager(), "https://xsnexplorer.io/api/"));
    XSNBlockExplorerHttpClient XSNBlockExplorerApi(std::move(requestHandler));;

    //    QSignalSpy spy(&XSNBlockExplorerApi, &XSNBlockExplorerHttpClient::sendTransactionFinished);
    //    const QString address = "XcqpUChZhNkVDgQqFF9U4DdewDGUMWwG53";
    //XSNBlockExplorerApi.sendTransactionsByAddress(address);

    //    spy.wait(3000);
    //EXPECT_EQ(spy.count(), 1);
    //QList<QVariant> arguments = spy.takeFirst();
    //EXPECT_TRUE(arguments.at(0).toString().contains("a16b3b22a7e6ec9b7b969f933aa33dcaef22da2ce22d75b9566d466b04cef755"));
}

TEST_F(WalletTests, syncAddress)
{
    std::unique_ptr<RequestHandlerImpl> requestHandler(new RequestHandlerImpl(new QNetworkAccessManager(), "https://xsnexplorer.io/api/"));
    XSNBlockExplorerHttpClient XSNBlockExplorerApi(std::move(requestHandler));
    ChainSyncHelper syncHelper(&XSNBlockExplorerApi, assetsModel.assetById(384));
}

TEST_F(WalletTests, syncHeadersBatch)
{
    auto client = _ntFactory->createBlockExplorerClient(384);
    ChainSyncHelper syncHelper(client.get(), assetsModel.assetById(384));
    std::vector<BlockHeader> syncedHeaders;
    QObject::connect(&syncHelper, &ChainSyncHelper::headersSynced, [&syncedHeaders](std::vector<std::tuple<BlockHeader, EncodedBlockFilter>> synced) {
        std::transform(std::begin(synced), std::end(synced),
                       std::back_inserter(syncedHeaders), [](const auto &entry) {
            return std::get<0>(entry);
        });
    });

    syncHelper.getLastHeaders(QString());

    QSignalSpy spy(&syncHelper, &ChainSyncHelper::headersSynced);
    spy.wait();
    ASSERT_EQ(spy.count(), 1);
    ASSERT_FALSE(syncedHeaders.empty());

    BlockHash prevHeaderHash;
    for(auto &&header : syncedHeaders)
    {
        if(!prevHeaderHash.isEmpty())
        {
            ASSERT_EQ(prevHeaderHash, header.prevBlockHash);
        }

        prevHeaderHash = header.hash;
    }
}

TEST_F(WalletTests, syncBestBlockHash)
{
    auto client = _ntFactory->createBlockExplorerClient(_xsnAsset.coinID());
    ChainSyncHelper syncHelper(client.get(), assetsModel.assetById(_xsnAsset.coinID()));

    syncHelper.getBestBlockHash();

    QSignalSpy spy(&syncHelper, &ChainSyncHelper::bestBlockSynced);
    spy.wait();
    ASSERT_EQ(spy.count(), 1);
    ASSERT_FALSE(spy.takeFirst().first().toString().isEmpty());
}

TEST_F(WalletTests, syncBlockTx)
{
    auto client = _ntFactory->createBlockExplorerClient(_xsnAsset.coinID());
    ChainSyncHelper syncHelper(client.get(), assetsModel.assetById(_xsnAsset.coinID()));

    StrippedBlock strippedBlock;
    QObject::connect(&syncHelper, &ChainSyncHelper::blockSynced, [&strippedBlock](StrippedBlock block) {
        strippedBlock = block;
    });

    QSignalSpy spy(&syncHelper, &ChainSyncHelper::blockSynced);
    QString blockHash("7ab9fec75d616cdf4f4b50aa79289c3eb290d1d3791569312943b8fc2ea7ad4c");
    syncHelper.getLightWalletBlock(blockHash, 523331);

    spy.wait();
    ASSERT_EQ(spy.count(), 1);
    ASSERT_EQ(blockHash, strippedBlock.hash);
    ASSERT_EQ(strippedBlock.transactions.size(), 8);
}

TEST_F(WalletTests, syncManagerStart)
{
    Chain chain(_xsnAsset.coinID(), [](const auto &, const auto &) { });
    auto syncManager = _smFactory->createAPISyncManager(chain);
    QSignalSpy spy(syncManager.get(), &AbstractChainSyncManager::isSyncingChanged);
    syncManager->trySync();
    spy.wait(500);
    ASSERT_EQ(spy.count(), 1);
}

TEST_F(WalletTests, syncManagerStop)
{
#if 0
    Chain chain(_xsnAsset.coinID());
    auto syncManager = _smFactory->createAPISyncManager(chain);
    QSignalSpy spy(syncManager.get(), &AbstractChainSyncManager::isSyncingChanged);
    syncManager->trySync();
    spy.wait(50);
    ASSERT_EQ(spy.count(), 1);
    syncManager->interrupt();
    spy.wait(50);
    ASSERT_EQ(spy.count(), 2);
#endif
}

TEST_F(WalletTests, syncManagerTrySync)
{
#if 0
    Chain chain(_xsnAsset.coinID());
    auto syncManager = _smFactory->createAPISyncManager(chain);
    QSignalSpy spy(syncManager.get(), &AbstractChainSyncManager::isSyncingChanged);
    syncManager->trySync();
    spy.wait(50);
    ASSERT_EQ(spy.count(), 1);
    spy.wait(1000);
    syncManager->interrupt();
    spy.wait(50);
    ASSERT_EQ(spy.count(), 2);
    ASSERT_GT(chain.getHeight(), 0);
#endif
}

TEST_F(WalletTests, syncManagerSyncBlock)
{
#if 0
    Chain chain(_xsnAsset.coinID());
    const QString testBlockHash("00000426cd6cbea021ec182e730ee25cdc96987b4709b70aee3ffe7605471ab9");
    MockDataSource::AddressMap testAddresses;
    testAddresses[_xsnAsset.coinID()].insert(QString("XnH3bC9NruJ4wnu4Dgi8F3wemmJtcxpKp6"));
    _mockDataSource->setAddressMap(testAddresses);
    _filterMatcher->matches.insert(testBlockHash);

    auto syncManager = _smFactory->createAPISyncManager(chain);
    syncManager->trySync();

    QSignalSpy spy(syncManager.get(), &AbstractChainSyncManager::isSyncingChanged);
    spy.wait(2000);

    syncManager->interrupt();

    const std::vector<QString> testTxHashes {
        "d6cc15a22d91731d1f82ed0ca393382e6d112fb08afb0df7af7459b787f6651d"
    };
    ASSERT_TRUE(chain.lookup(testBlockHash));

    auto transactions = _cache->transactionsListByID(_xsnAsset.coinID());

    for(auto &&testTxHash : testTxHashes)
    {
        auto transaction = TransactionUtils::FindTransaction(transactions, testTxHash);
        ASSERT_TRUE(transaction);
        ASSERT_GT(transaction->outputs().size(), 0);
    }
#endif
}

#if 0
TEST_F(WalletTests, stopSyncService)
{
    SyncService syncService(*_smFactory, *_chainManager, assetsModel);

    syncService.start();

    QSignalSpy spy(&syncService, &SyncService::syncStopped);

    syncService.stop();
    if(spy.count() == 0)
    {
        ASSERT_TRUE(spy.wait(1000));
    }
}

TEST_F(WalletTests, syncServiceTaskStarted)
{
    _chainManager->addTestChains({_xsnAsset.coinID()});
    _chainManager->loadChains({_xsnAsset.coinID()});
    SyncService syncService(*_smFactory, *_chainManager, assetsModel);

    syncService.start();

    QSignalSpy spy(&syncService, &SyncService::syncTaskStarted);

    ASSERT_TRUE(spy.wait(3000));

    syncService.stop();

    ASSERT_EQ(spy.takeFirst().front().value<AssetID>(), _xsnAsset.coinID());
}
#endif
TEST_F(WalletTests, taskFinished)
{
#if 0
    auto xsnAssetID = _xsnAsset.coinID();
    auto ltcAssetID = _ltcAsset.coinID();
    MockDataSource dataSource(true, true); // already loaded and created
    MockDataSource::AddressMap expectedAddresses {
        {xsnAssetID, { {"7Tjqc6cCTVWHiiJV57S7qdMQYiXgWPeRfS", "XxW5G1VpeiorBKQcuQWia66WSqEypCXFtS", "XbTAKPneciQVXD4F8q66LAPCk4jgDdcfoV"}, 0},
        },
        {
            ltcAssetID, {{"LNkDW3DCZ6M2NXdKVBgquExARK46iAmDdK", "ltc1qy6se8xv5hg79qz9awuvc7q0h23qvxzl8w8d34w", "3GZwdQbLmHg8f6qFjQ8rmyRQtz4iAodv5G"}, 0 }
        }
    };
    dataSource.setAddressMap(expectedAddresses);

    SyncService syncService(_smFactory.get(), &dataSource, &assetsModel);

    QSignalSpy startedSpy(&syncService, &SyncService::syncTaskStarted);
    QSignalSpy finishedSpy(&syncService, &SyncService::syncTaskFinished);

    syncService.start();
    startedSpy.wait(5000);
    ASSERT_EQ(startedSpy.count(), 1);

    ASSERT_TRUE(finishedSpy.wait(12000));
    finishedSpy.wait(10000);
    ASSERT_EQ(finishedSpy.count(), 2);
    finishedSpy.wait(10000);
    ASSERT_EQ(finishedSpy.count(), 3);
    finishedSpy.wait(13000);
#endif
}

TEST_F(WalletTests, initialSync)
{
#if 0
    auto xsnAssetID = _xsnAsset.coinID();
    auto ltcAssetID = _ltcAsset.coinID();
    MockDataSource dataSource(true, true); // already loaded and created
    MockDataSource::AddressMap expectedAddresses {
        {xsnAssetID, { {"7Tjqc6cCTVWHiiJV57S7qdMQYiXgWPeRfS", "XxW5G1VpeiorBKQcuQWia66WSqEypCXFtS", "XbTAKPneciQVXD4F8q66LAPCk4jgDdcfoV"}, 0},
        },
        {
            ltcAssetID, {{"LNkDW3DCZ6M2NXdKVBgquExARK46iAmDdK", "ltc1qy6se8xv5hg79qz9awuvc7q0h23qvxzl8w8d34w", "3GZwdQbLmHg8f6qFjQ8rmyRQtz4iAodv5G"}, 0 }
        }
    };
    dataSource.setAddressMap(expectedAddresses);

    SyncService syncService(_smFactory.get(), &dataSource, &assetsModel);
    TransactionsList syncedTransactions;
    ChainSyncManager &syncManager = syncService.chainSyncManager(384);
    //    QObject::connect(&syncManager, &ChainSyncManager::transactionSynced, [&syncedTransactions](TransactionEntryPtr entry) {
    //        syncedTransactions.emplace_back(entry);
    //    });

    QSignalSpy syncStartedSpy(&syncService, &SyncService::syncTaskStarted);
    QSignalSpy syncFinishedSpy(&syncService, &SyncService::syncTaskFinished);

    syncService.start();

    // as our mock is empty, we should request all transactions.
    if(syncStartedSpy.count() == 0)
    {
        ASSERT_TRUE(syncStartedSpy.wait(5000));
    }

    ASSERT_TRUE(syncFinishedSpy.wait(10000));
    //    AssetID gotID;
    //    QObject::connect(&dataSource, &WalletDataSource::allTransactionsFetched, [&gotID](AssetID id) {
    //        gotID = id;
    //    });
    //    dataSource.fetchAllTransactions();
    //    QSignalSpy(&dataSource, &WalletDataSource::allTransactionsFetched).wait();
    //    ASSERT_EQ(assetID, gotID);
#endif
}

TEST_F(WalletTests, parsingOfDoubleAmount)
{
#if 0
    std::unique_ptr<RequestHandlerImpl> requestHandler(new RequestHandlerImpl(new QNetworkAccessManager(), "https://xsnexplorer.io/api/"));
    XSNBlockExplorerHttpClient XSNBlockExplorerApi(std::move(requestHandler));

    QSignalSpy spy(&XSNBlockExplorerApi, &XSNBlockExplorerHttpClient::getTransactionsForAddressFinished);
    const QString address = "7Tjqc6cCTVWHiiJV57S7qdMQYiXgWPeRfS";
    XSNBlockExplorerApi.getTransactionsForAddress(address, 2, "asc", AssetID(384));

    spy.wait(10000);
    ASSERT_EQ(spy.count(), 1);

    QList<QVariant> arguments = spy.takeFirst();
    QJsonDocument itemDoc = QJsonDocument::fromJson(arguments.at(2).toByteArray());
    QJsonObject rootObject = itemDoc.object();
    QJsonArray data = rootObject.value("data").toArray();
    QJsonObject firstTransaction = data[0].toObject();
    //    ASSERT_EQ(ParseAmount(firstTransaction.value("outputs").toArray().first().toObject().value("value")), 17324594613619);
#endif
}

struct TransactionEntryPtrComparator {
    bool operator()(const TransactionRef &lhs, const TransactionRef &rhs) const
    {
        return lhs->txId() == rhs->txId();
    }
};

using TransactionsSet = std::set<TransactionRef, TransactionEntryPtrComparator>;

TEST_F(WalletTests, mergingTransactionEntry)
{
#if 0
    auto xsnAssetID = _xsnAsset.coinID();
    auto ltcAssetID = _ltcAsset.coinID();
    MockDataSource dataSource(true, true); // already loaded and created
    MockDataSource::AddressMap expectedAddresses {
        {xsnAssetID, { {"7Tjqc6cCTVWHiiJV57S7qdMQYiXgWPeRfS", "XxW5G1VpeiorBKQcuQWia66WSqEypCXFtS", "XbTAKPneciQVXD4F8q66LAPCk4jgDdcfoV"}, 0},
        },
        {
            ltcAssetID, {{"LNkDW3DCZ6M2NXdKVBgquExARK46iAmDdK", "ltc1qy6se8xv5hg79qz9awuvc7q0h23qvxzl8w8d34w", "3GZwdQbLmHg8f6qFjQ8rmyRQtz4iAodv5G"}, 0 }
        }
    };

    dataSource.setAddressMap(expectedAddresses);
    SyncService syncService(_smFactory.get(), &dataSource, &assetsModel);
    ChainSyncManager& syncManager = syncService.chainSyncManager(384);

    std::map<QString, TransactionsSet> syncedTransactions;
    QObject::connect(&syncManager, &ChainSyncManager::transactionSynced, [&syncedTransactions](TransactionEntryPtr entry) {
        auto address = std::get<0>(entry->transactionData());
        syncedTransactions[address].insert(entry);
    });

    QSignalSpy syncFinishedSpy(&syncService, &SyncService::syncTaskFinished);
    syncService.start();

    ASSERT_TRUE(syncFinishedSpy.wait(10000));
    syncFinishedSpy.wait(10000);
    ASSERT_EQ(syncFinishedSpy.count(), 2);
    syncFinishedSpy.wait(10000);
    ASSERT_EQ(syncFinishedSpy.count(), 3);
    syncFinishedSpy.wait(13000);

    TransactionsSet commonSet;
    const auto &firstSet = syncedTransactions.at("7Tjqc6cCTVWHiiJV57S7qdMQYiXgWPeRfS");
    const auto &secondSet = syncedTransactions.at("XxW5G1VpeiorBKQcuQWia66WSqEypCXFtS");
    std::set_intersection(std::begin(firstSet), std::end(firstSet),
                          std::begin(secondSet), std::end(secondSet),
                          std::inserter(commonSet, std::begin(commonSet)), TransactionEntryPtrComparator());

    ASSERT_FALSE(commonSet.empty());

    for(auto &&entry : syncedTransactions)
    {
        for(const auto &transaction : entry.second)
        {
            // means that transaction has to be merged
            if(commonSet.count(transaction) == 1)
            {
                const auto &storedTransactionEntry = TransactionEntryUtils::FindTransaction(
                            dataSource.transactionsListByID(transaction->assetID()), transaction->txId());

                auto data = transaction->transactionData();
                auto syncedAddress = std::get<0>(data);
                auto syncedInputs = std::get<1>(data);
                auto syncedOutputs = std::get<2>(data);

                auto storedBitcoinTx = storedTransactionEntry->bitcoinTx();
                for(auto &&input : syncedInputs)
                {
                    bitcoin::CTxInDB expectedIn(std::get<0>(input).toStdString(), std::get<1>(input), std::get<2>(input), syncedAddress.toStdString());
                    ASSERT_NE(std::find(std::begin(storedBitcoinTx.vIn), std::end(storedBitcoinTx.vIn), expectedIn),
                              std::end(storedBitcoinTx.vIn));
                }

                for(auto &&output : syncedOutputs)
                {
                    auto index = std::get<0>(output);
                    bitcoin::CTxOutDB expectedOut(index, syncedAddress.toStdString(), std::get<1>(output));
                    ASSERT_TRUE(Utils::Exists(storedBitcoinTx.vOut, expectedOut));
                }
            }
        }
    }
#endif
}

TEST_F(WalletTests, syncProcess)
{
#if 0
    auto xsnAssetID = _xsnAsset.coinID();
    auto ltcAssetID = _ltcAsset.coinID();
    MockDataSource dataSource(true, true); // already loaded and created
    MockDataSource::AddressMap expectedAddresses {
        {xsnAssetID, { {"7Tjqc6cCTVWHiiJV57S7qdMQYiXgWPeRfS", "XxW5G1VpeiorBKQcuQWia66WSqEypCXFtS", "XbTAKPneciQVXD4F8q66LAPCk4jgDdcfoV"}, 0},
        },
        {
            ltcAssetID, {{"LNkDW3DCZ6M2NXdKVBgquExARK46iAmDdK", "ltc1qy6se8xv5hg79qz9awuvc7q0h23qvxzl8w8d34w", "3GZwdQbLmHg8f6qFjQ8rmyRQtz4iAodv5G"}, 0 }
        }
    };

    dataSource.setAddressMap(expectedAddresses);

    SyncService syncService(_smFactory.get(), &dataSource, &assetsModel);

    QSignalSpy syncStartedSpy(&syncService, &SyncService::syncTaskStarted);
    QSignalSpy syncFinishedSpy(&syncService, &SyncService::syncTaskFinished);
    QSignalSpy syncStopedSpy(&syncService, &SyncService::syncStopped);

    syncService.start();

    // as our mock is empty, we should request all transactions.
    if(syncStartedSpy.count() == 0)
    {
        ASSERT_TRUE(syncStartedSpy.wait(5000));
    }

    //syncService.stop();
    //syncService.start();
    ASSERT_EQ(syncStartedSpy.count(), 1);
    syncFinishedSpy.wait(10000);
    ASSERT_EQ(syncFinishedSpy.count(), 1);
    syncFinishedSpy.wait(10000);
    ASSERT_EQ(syncFinishedSpy.count(), 2);
    syncFinishedSpy.wait(13000);
    auto id = dataSource.transactionsListByID(xsnAssetID).back().get()->txId();
    ASSERT_EQ(id, "3a2d7e0bda8c882c526ce7532961ecf1943c0a304a9c4aab774e8f5b5b077a37");
#endif
}

TEST_F(WalletTests, OutputTest)
{
    WalletAssetsModel assetsModel("assets_conf.json");
    auto btcCoin = assetsModel.assetById(0);
    std::unique_ptr<RequestHandlerImpl> requestHandler(new RequestHandlerImpl(new QNetworkAccessManager(), "https://xsnexplorer.io/api/btc"));
    XSNBlockExplorerHttpClient *XSNBlockExplorerApi = new XSNBlockExplorerHttpClient(std::move(requestHandler));
    ChainSyncHelper chainHelper(XSNBlockExplorerApi, btcCoin);
    chainHelper.getLightWalletBlock("000000000000000000015c4f1ad693f2ba9e0b5717011b5b00bd5ad494a260e2", 570711);

    QSignalSpy spy(&chainHelper, &ChainSyncHelper::blockSynced);

    spy.wait(80000);

    auto blocks = spy.takeFirst().at(0);
}

#endif

TEST_F(WalletTests, ReorgTest)
{
    const AssetID assetID = 384;
    RegtestDataSource regtestDataSource(assetsModel);
    RegtestChainManager regtestChainManager(regtestDataSource, assetsModel);
    regtestChainManager.loadChains({ assetID }).wait();

    std::map<size_t, Wire::VerboseBlockHeader> connectedBlocks;

    auto set = [&connectedBlocks](
                   auto verboseHeader) { connectedBlocks[verboseHeader.height] = verboseHeader; };
    auto get = [&connectedBlocks](auto height) -> boost::optional<Wire::VerboseBlockHeader> {
        return connectedBlocks.count(height) > 0 ? boost::make_optional(connectedBlocks.at(height))
                                                 : boost::none;
    };

    Chain localChain(assetID, set, get);
    ChainSyncManager syncManager(localChain, *_cache, *_mockDataSource, *_filterMatchable,
        assetsModel.assetById(assetID), regtestDataSource);

    regtestDataSource.chain(assetID).generateBlocks(bitcoin::CScript(), 10000);

    syncManager.trySync();
    QSignalSpy firstSyncFinished(&syncManager, &ChainSyncManager::finished);
    ASSERT_TRUE(firstSyncFinished.wait());

    const auto& bestChain = regtestDataSource.chain(assetID);
    ASSERT_EQ(localChain.bestBlockHash().toStdString(), bestChain.bestBlockHash());
    regtestDataSource.chain(assetID).reorganizeChain(bitcoin::CScript(), 12, 20);
    ASSERT_NE(localChain.bestBlockHash().toStdString(), bestChain.bestBlockHash());

    QSignalSpy secondSyncFinished(&syncManager, &ChainSyncManager::finished);
    syncManager.trySync();
    ASSERT_TRUE(secondSyncFinished.wait());
    ASSERT_EQ(localChain.bestBlockHash().toStdString(), bestChain.bestBlockHash());
}

static chain::TxOutpoint GenerateOutpoint(std::string hash, uint32_t index)
{
    chain::TxOutpoint r;
    r.set_hash(hash);
    r.set_index(index);
    return r;
}

static chain::TxOutput GenerateOutput(uint32_t index, int64_t amount, std::string address)
{
    chain::TxOutput r;
    r.set_index(index);
    r.set_value(amount);
    r.set_address(address);
    return r;
}

TEST_F(WalletTests, TransactionDecoding)
{
    const std::string encodedTx
        = "0100000000010109ce220cd03ba4db85f7f48772926c67ecd2cbd535318752982778e127014e050000000000"
          "ffffffff02ffffff0000000000220020781fb0920f24dfd542e3cda671dfd1ddbe5b6bfcc4db91dd3983fa0a"
          "fd57fbf384bddd26000000001600141a2c003aeeeff7eba0a3d3cfd0c6a0e2a04aff0d0247304402207e9626"
          "fe74aeb68b9a61206a46494178d928c1042f427c4635b9e6d361ec408b02202bca125c4279476ce5be4f06d7"
          "35f9b2e142ad134e2fa9e7d424861e64125391012103ad4a20ffd8d68d8c7f3d3b9c37362664631e940a1ab1"
          "da59821e9915a26da28500000000";

    auto mutableTx = MutableTransaction::FromRawTx(384, encodedTx);
    auto tx = TransactionUtils::MutableTxToTransactionRef(mutableTx, assetsModel);
    ASSERT_EQ(tx->outputs().size(), 2);

    OnChainTx::Outputs expectedOutputs
        = { GenerateOutput(
                0, 16777215, "xc1q0q0mpys0yn0a2shrekn8rh73mkl9k6lucnderhfes0aq4l2hl0es02ma08"),
              GenerateOutput(1, 652066180, "xc1qrgkqqwhwalm7hg9r608ap34qu2sy4lcdvdpavn") };

    ASSERT_EQ(tx->outputs().size(), expectedOutputs.size());

    for (size_t i = 0; i < expectedOutputs.size(); ++i) {
        const auto& output = tx->outputs().at(i);
        ASSERT_EQ(output.address(), expectedOutputs.at(i).address());
        ASSERT_EQ(output.index(), expectedOutputs.at(i).index());
        ASSERT_EQ(output.value(), expectedOutputs.at(i).value());
    }

    OnChainTx::Inputs inputs = { GenerateOutpoint(
        "054e0127e178279852873135d5cbd2ec676c927287f4f785dba43bd00c22ce09", 0) };

    ASSERT_EQ(tx->inputs().size(), inputs.size());

    for (size_t i = 0; i < inputs.size(); ++i) {
        const auto& input = tx->inputs().at(i);
        ASSERT_EQ(input.hash(), inputs.at(i).hash());
        ASSERT_EQ(input.index(), inputs.at(i).index());
    }

    ASSERT_EQ("a6f5c99ab412841ab2ea1ba1fd6ea27b3379e69bbf0e4f502886256fb542dd8f", tx->txId());
}

TEST_F(WalletTests, GetTxOut)
{
    auto client = _ntFactory->createBlockExplorerClient(384);
    QString txid = "507e07df1b0a6b34d19c71cd2468cee990bed14cf91590feba4d90f2fff36e51";
    client->getTxOut(txid, 3)
        .then([](QByteArray data) {
            return Wire::TxOut::FromJson(QJsonDocument::fromJson(data).object());
        })
        .then([](Wire::TxOut txout) {
            ASSERT_EQ("76a9148791b32cc84e0a63f6b63192ef04fe0f4f8d9eb688ac",
                bitcoin::HexStr(txout.pkScript));
            ASSERT_EQ(int64_t(900000000), txout.value);
        })
        .fail([] { ASSERT_TRUE(false); })
        .wait();

    client->getTxOut(txid, 4)
        .then([](QByteArray) { ASSERT_TRUE(false); })
        .fail([](NetworkUtils::ApiErrorException) { ASSERT_TRUE(true); })
        .wait();
}

TEST_F(WalletTests, BlockFilter)
{
    auto client = _ntFactory->createBlockExplorerClient(384);
    QString hash("7ee45df33ac4f3b37e7cfc9ec3a40856c8d636296531e78f7b863e653bbe8e93");
    std::vector<std::string> outpoints;

    ASSERT_FALSE(client->getBlockFilter(hash)
                     .then([](QByteArray rawData) {
                         return Wire::EncodedBlockFilter::FromJson(
                             QJsonDocument::fromJson(rawData).object().value("filter").toObject());
                     })
                     .then([hash, outpoints](Wire::EncodedBlockFilter filter) {
                         bitcoin::Filter::key_t key;
                         auto hashBlock = bitcoin::uint256S(hash.toStdString());
                         std::memcpy(&key[0], hashBlock.begin(), key.size());
                         bitcoin::Filter gcs = bitcoin::Filter::FromNMPBytes(
                             key, filter.n, filter.m, filter.p, filter.bytes);
                         ASSERT_TRUE(gcs.matchAny(outpoints));
                     })
                     .wait()
                     .isRejected());
}

#endif // TST_PORTALHTTPCLIENT_HPP
