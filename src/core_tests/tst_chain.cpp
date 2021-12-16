//#ifndef TST_CHAIN_HPP
//#define TST_CHAIN_HPP
//#include <gtest/gtest.h>
//#include <QJsonDocument>
//#include <QJsonArray>
//#include <QJsonObject>
//#include <QFile>
//#include <QDir>
//#include <QStandardPaths>
//#include <Chain/Chain.hpp>
//#include <Chain/BlockHeader.hpp>
//#include <Data/WalletAssetsModel.hpp>
//#include <txdb.h>
//#include <Tools/DBUtils.hpp>

// using namespace testing;

// class ChainTests : public ::testing::Test
//{
// public:
//    ChainTests() : assetsModel("assets_conf.json")
//    {
//        QFile file("testdata/chain_data_1.json");
//        if(file.open(QFile::ReadOnly))
//        {
//            for(auto &&obj : QJsonDocument::fromJson(file.readAll()).array())
//            {
//                headers.emplace_back(BlockHeader::FromJson(obj.toObject()));
//            }
//        }

//        fillTransactions();

//        _tempPath = QDir(
//                    QStandardPaths::writableLocation(
//                        QStandardPaths::TempLocation)).absoluteFilePath("chainTests");
//    }

//    void SetUp() override
//    {
//        _indexPath = setupTestFolder("index");
//        _txDbPath = setupTestFolder("txdb");
//    }

//    void TearDown() override
//    {
//    }

//    QDir setupTestFolder(QString folderName)
//    {
//        QDir folder;
//        folder.setPath(_tempPath);
//        if(folder.exists())
//        {
//            folder.removeRecursively();
//        }

//        folder.mkpath(".");
//        return folder;
//    }

//    void fillTransactions()
//    {
//        QFile file("testdata/tx_data_1.json");
//        if(file.open(QFile::ReadOnly))
//        {
//            for(auto &&value : QJsonDocument::fromJson(file.readAll()).array())
//            {
//                auto obj = value.toObject();
//                QString id = obj.value("id").toString();
//                qint64 timestamp = static_cast<qint64>(obj.value("time").toDouble());

//                Inputs inputs;
//                for(auto value : obj.value("inputs").toArray())
//                {
//                    inputs.push_back(Outpoint::FromJson(value.toObject()));
//                }

//                Outputs outputs;
//                for(auto value : obj.value("outputs").toArray())
//                {
//                    outputs.push_back(Output::FromJson(value.toObject()));
//                }

//                QString blockHash = obj.value("blockHash").toString();
//                int64_t blockHeight = static_cast<int64_t>(obj.value("height").toDouble());

//                transactions.emplace_back(testAssetID(),
//                                          id,
//                                          blockHeight,
//                                          QDateTime::fromSecsSinceEpoch(timestamp),
//                                          inputs, outputs);
//            }
//        }
//    }

//    const CoinAsset &testAsset() const
//    {
//        return assetsModel.assetById(384);
//    }

//    AssetID testAssetID() const
//    {
//        return testAsset().coinID();
//    }

// protected:
//    WalletAssetsModel assetsModel;
//    std::vector<BlockHeader> headers;
//    std::vector<Transaction> transactions;
//    QDir _indexPath;
//    QDir _txDbPath;
//    QString _tempPath;

//};

// TEST_F(ChainTests, parsingOk)
//{
//    ASSERT_EQ(headers.front().height, 0);
//    ASSERT_GE(headers.front().timestamp, 0);
//    ASSERT_FALSE(headers.front().hash.isEmpty());
//    ASSERT_TRUE(headers.front().prevBlockHash.isEmpty());
//    ASSERT_FALSE(headers.back().prevBlockHash.isEmpty());
//}

//#if 0

// TEST_F(ChainTests, fillChain)
//{

//    Chain chain(testAssetID());
//    for(auto header : headers)
//    {
//        auto index = chain.addToIndex(BlockIndex(header, {}));
//        index->setPrevIndex(chain.tip());
//        chain.setTip(index);
//        ASSERT_EQ(chain.tip()->header().hash, header.hash);
//    }
//}

// TEST_F(ChainTests, lookupChain)
//{
//    Chain chain(testAssetID());
//    for(auto header : headers)
//    {
//        auto index = chain.addToIndex(BlockIndex(header, {}));
//        index->setPrevIndex(chain.tip());
//        chain.setTip(index);
//        ASSERT_EQ(chain.tip()->header().hash, header.hash);
//    }

//    const auto &index = chain.lookup(headers.at(1).hash);
//    ASSERT_EQ(headers.at(1).hash, index->header().hash);
//}

// TEST_F(ChainTests, replaceTip)
//{
//    Chain chain(testAssetID());
//    for(auto header : headers)
//    {
//        auto index = chain.addToIndex(BlockIndex(header, {}));
//        index->setPrevIndex(chain.tip());
//        chain.setTip(index);
//    }

//    auto secondIndex = chain.lookup(headers.at(1).hash);

//    ASSERT_NE(chain.tip()->header().hash, secondIndex->header().hash);
//    chain.setTip(secondIndex);
//    ASSERT_EQ(chain.tip()->header().hash, secondIndex->header().hash);
//}

// TEST_F(ChainTests, bestBlockHash)
//{
//    Chain chain(testAssetID());
//    for(auto header : headers)
//    {
//        auto index = chain.addToIndex(BlockIndex(header, {}));
//        index->setPrevIndex(chain.tip());
//        chain.setTip(index);
//        ASSERT_EQ(chain.tip()->header().hash, header.hash);
//    }

//    ASSERT_EQ(headers.back().hash, chain.bestBlockHash());
//}

// TEST_F(ChainTests, blockHeightByHash)
//{
//    Chain chain(testAssetID());
//    for(auto header : headers)
//    {
//        auto index = chain.addToIndex(BlockIndex(header, {}));
//        index->setPrevIndex(chain.tip());
//        chain.setTip(index);
//        ASSERT_EQ(chain.tip()->header().hash, header.hash);
//    }

//    ASSERT_EQ(chain.getBlockHeight(headers.at(1).hash).get_value_or(0), 1);
//}

// TEST_F(ChainTests, setOnlyTip)
//{
//    Chain chain(testAssetID());
//    BlockIndexPtr pindexPrev = nullptr;
//    for(auto header : headers)
//    {
//        auto index = chain.addToIndex(BlockIndex(header, {}));
//        index->setPrevIndex(pindexPrev);
//        pindexPrev = index;
//    }

//    chain.setTip(pindexPrev);

//    for(size_t i = 0; i < headers.size(); ++i)
//    {
//        ASSERT_EQ(headers.at(i).hash, chain.at(i)->header().hash);
//    }

//}

// TEST_F(ChainTests, saveAndLoadChainOnDisk)
//{
//    Chain chain(testAssetID());
//    BlockIndexPtr pindexPrev = nullptr;
//    for(auto header : headers)
//    {
//        auto index = chain.addToIndex(BlockIndex(header, {}));
//        index->setPrevIndex(pindexPrev);
//        pindexPrev = index;
//    }

//    chain.setTip(pindexPrev);

//    std::vector<std::pair<AssetID, bitcoin::CDiskBlockIndex>> diskIndexes;

//    auto pindex = chain.tip();
//    while(pindex)
//    {
//        diskIndexes.emplace_back(chain.assetID(), DBUtils::BlockIndexToDisk(*pindex));
//        pindex = pindex->prev();
//    }

//    {
//        bitcoin::CBlockTreeDB blockIndexDb(_indexPath.absolutePath().toStdString(), 1000);
//        ASSERT_TRUE(blockIndexDb.WriteBatchSync(diskIndexes));
//    }

//    Chain loadedChain(chain.assetID());

//    {
//        std::vector<BlockIndexPtr> sortedByHeight;
//        bitcoin::CBlockTreeDB blockIndexDb(_indexPath.absolutePath().toStdString(), 1000);
//        blockIndexDb.LoadBlockIndexGuts([&loadedChain, &sortedByHeight](AssetID assetID, const
//        bitcoin::CDiskBlockIndex &diskIndex) {
//            ASSERT_EQ(assetID, loadedChain.assetID());

//            auto convertedIndex = DBUtils::DiskIndexToBlockIndex(diskIndex);

//            BlockIndexPtr index = loadedChain.addToIndex(convertedIndex);
//            *index = convertedIndex;
//            if(!convertedIndex.header().prevBlockHash.isEmpty())
//            {
//                BlockHeader prevBlockHeader(convertedIndex.header().prevBlockHash, QString(), 0,
//                0); index->setPrevIndex(loadedChain.addToIndex(BlockIndex(prevBlockHeader, {})));
//            }

//            sortedByHeight.push_back(index);
//        });

//        std::sort(std::begin(sortedByHeight), std::end(sortedByHeight), [](const BlockIndexPtr
//        &left, const BlockIndexPtr &right) {
//            return left->height() > right->height();
//        });

//        loadedChain.setTip(sortedByHeight.front());
//    }

//    for(size_t i = 0; i <= chain.getHeight(); ++i)
//    {
//        ASSERT_EQ(chain.at(i)->header().hash, loadedChain.at(i)->header().hash);
//    }
//}

//#endif

// TEST_F(ChainTests, saveAndLoadTxOnDisk)
//{
//    using namespace bitcoin;
//    std::vector<CDiskTx> diskTransactions;
//    std::transform(std::begin(transactions), std::end(transactions),
//                   std::back_inserter(diskTransactions), DBUtils::TransactionToDiskTx);

//    std::map<QString, Transaction> loadedTransactions;

//    {
//        bitcoin::CTxDB txDb(_txDbPath.absolutePath().toStdString(), 1000);
//        ASSERT_TRUE(txDb.WriteBatchSync(diskTransactions));
//    }

//    {
//        auto assetID = testAssetID();
//        bitcoin::CTxDB txDb(_txDbPath.absolutePath().toStdString(), 1000);
//        ASSERT_TRUE(txDb.LoadTxDBGuts([&loadedTransactions, assetID](const CDiskTx &diskTx) {
//            ASSERT_EQ(diskTx.assetID, assetID);
//            auto transaction = DBUtils::DiskTxToTransaction(diskTx);
//            loadedTransactions.emplace(transaction.txId(), transaction);
//        }));
//    }

//    ASSERT_EQ(transactions.size(), loadedTransactions.size());

//    for(auto &&transaction : transactions)
//    {
//        ASSERT_EQ(loadedTransactions.count(transaction.txId()), 1);
//        auto loadedTx = loadedTransactions.at(transaction.txId());
//        ASSERT_EQ(loadedTx, transaction);
//        ASSERT_EQ(loadedTx.inputs(), transaction.inputs());
//        ASSERT_EQ(loadedTx.outputs(), transaction.outputs());
//    }
//}

//#endif // TST_CHAIN_HPP
