#include "RegtestChain.hpp"
#include <Chain/Chain.hpp>
#include <Data/WalletAssetsModel.hpp>
#include <Networking/AbstractBlockExplorerHttpClient.hpp>
#include <Networking/NetworkingUtils.hpp>
#include <Utils/Logging.hpp>
#include <arith_uint256.h>
#include <golomb/gcs.h>
#include <hash.h>
#include <key_io.h>
#include <script/script.h>
#include <streams.h>
#include <transaction.h>
#include <utilstrencodings.h>
#include <utiltime.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>

using namespace bitcoin;

//==============================================================================

CTransaction ConvertToBitcoinTx(Wire::MsgTx msgTx)
{
    CMutableTransaction tx;
    tx.nVersion = msgTx.version;
    tx.nLockTime = msgTx.lockTime;
    for (auto&& in : msgTx.txIn) {
        tx.vin.push_back(CTxIn(COutPoint(in.previousOutPoint.hash, in.previousOutPoint.index),
            CScript(in.signatureScript.begin(), in.signatureScript.end()), in.sequence));
        tx.vin.back().scriptWitness.stack = in.witness;
    }
    for (auto&& out : msgTx.txOut) {
        tx.vout.push_back(CTxOut(out.value, CScript(out.pkScript.begin(), out.pkScript.end())));
    }

    return CTransaction(tx);
}

//==============================================================================

Wire::MsgTx ConvertToWireTx(const CTransaction& tx)
{
    Wire::MsgTx msgTx;
    msgTx.lockTime = tx.nLockTime;
    msgTx.version = tx.nVersion;
    for (auto&& in : tx.vin) {
        msgTx.txIn.push_back(Wire::TxIn{ Wire::OutPoint{ in.prevout.hash, in.prevout.n },
            std::vector<unsigned char>(in.scriptSig.begin(), in.scriptSig.end()),
            in.scriptWitness.stack, in.nSequence });
    }

    for (auto&& out : tx.vout) {
        msgTx.txOut.push_back(Wire::TxOut{ out.nValue,
            std::vector<unsigned char>(out.scriptPubKey.begin(), out.scriptPubKey.end()) });
    }

    return msgTx;
}

//==============================================================================

static unsigned int CalculateNextWorkRequired(const Wire::VerboseBlockHeader::Header& pindexLast)
{
    return pindexLast.bits;
}

//==============================================================================

static unsigned int GetNextWorkRequired(const Wire::VerboseBlockHeader::Header& pindexLast)
{
    return CalculateNextWorkRequired(pindexLast);
}

//==============================================================================

static bool CheckProofOfWork(bitcoin::uint256 hash, unsigned int nBits)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow)
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}

//==============================================================================

static void IncrementExtraNonce(
    Wire::MsgBlock& pblock, int currentHeight, unsigned int& nExtraNonce)
{
    // Update nExtraNonce
    static uint256 hashPrevBlock;
    //    if (hashPrevBlock != pblock->hashPrevBlock)
    //    {
    //        nExtraNonce = 0;
    //        hashPrevBlock = pblock->hashPrevBlock;
    //    }
    ++nExtraNonce;
    unsigned int nHeight
        = currentHeight + 1; // Height first in coinbase required for block.version=2
    //    CMutableTransaction txCoinbase(*pblock->vtx[0]);
    //    txCoinbase.vin[0].scriptSig = (CScript() << nHeight << CScriptNum(nExtraNonce));
    //    assert(txCoinbase.vin[0].scriptSig.size() <= 100);

    //    pblock->vtx[0] = MakeTransactionRef(std::move(txCoinbase));
    //    pblock->hashMerkleRoot = BlockMerkleRoot(*pblock);
}

//==============================================================================

std::vector<std::string> RegtestChain::generateBlocks(
    bitcoin::CScript coinbase_script, int nGenerate)
{
    static const int nInnerLoopCount = 0x10000;
    int nHeight = height();
    int nHeightEnd = nHeight + nGenerate;
    std::vector<std::string> result;
    unsigned int nExtraNonce = 0;

    if (coinbase_script.empty()) {
        static bitcoin::CScript defaultScript;
        if (defaultScript.empty()) {
            defaultScript = bitcoin::GetScriptForDestination(
                bitcoin::DecodeDestination("bc1q7dk898268jh6qyprnae2x7y8aafvspelr8hmel",
                    _assetsModel.assetById(_assetID).params().params));
        }
        coinbase_script = defaultScript;
    }

    while (nHeight < nHeightEnd) {
        auto newBlock = BlockAssembler(*this).CreateNewBlock(coinbase_script);

        for (auto&& tx : _mempool) {
            newBlock.transactions.emplace_back(tx);
        }

        _mempool.clear();

        {
            IncrementExtraNonce(newBlock, nHeight, nExtraNonce);
        }

        while (newBlock.header.nonce < nInnerLoopCount
            && !CheckProofOfWork(GetHash(newBlock.header), newBlock.header.bits)) {
            ++newBlock.header.nonce;
        }

        if (newBlock.header.nonce == nInnerLoopCount) {
            continue;
        }

        result.push_back(GetHashStr(newBlock.header));

        processNewBlock(newBlock);

        ++nHeight;
    }

    return result;
}

//==============================================================================

std::vector<std::string> RegtestChain::reorganizeChain(
    CScript coinbase_script, size_t blockToDisconnect, size_t blocksToConnect)
{
    if (blockToDisconnect > height()) {
        return {};
    }

    for (size_t i = 0; i < blockToDisconnect; ++i) {
        disconnectTip();
    }

    return blocksToConnect > 0 ? generateBlocks(coinbase_script, blocksToConnect)
                               : std::vector<std::string>();
}

//==============================================================================

void RegtestChain::processNewBlock(const Wire::MsgBlock& newBlock)
{
    _blocks.push_back(newBlock);
    auto blockHash = GetHashStr(newBlock.header);
    _blocksIndexByHash.emplace(blockHash, height() - 1);
    for (size_t j = 0; j < newBlock.transactions.size(); ++j) {
        auto tx = newBlock.transactions.at(j);
        auto txid = GetHash(tx);

        if (j > 0) {
            for (auto&& in : tx.txIn) {
                if (_unspentUTXOs.count(in.previousOutPoint) > 0) {
                    _unspentUTXOs.erase(in.previousOutPoint);
                }
            }
        }

        for (uint32_t i = 0; i < tx.txOut.size(); ++i) {
            _unspentUTXOs.emplace(Wire::OutPoint{ txid, i }, tx.txOut.at(i));
        }

        _transactionIndex.emplace(
            QString::fromStdString(txid.ToString()), QString::fromStdString(blockHash));
    }

    LogCDebug(General) << "Connecting tip" << QString::fromStdString(blockHash) << (height() - 1);
    tipConnected(QString::fromStdString(blockHash), height() - 1);
}

//==============================================================================

std::vector<Wire::MsgTx>& RegtestChain::mempool() const
{
    return _mempool;
}

//==============================================================================

const std::map<Wire::OutPoint, Wire::TxOut>& RegtestChain::utxoSet() const
{
    return _unspentUTXOs;
}

//==============================================================================

void RegtestChain::lockOutpoint(const Wire::OutPoint& outpoint)
{
    _lockedOutpoints.emplace(outpoint);
}

//==============================================================================

void RegtestChain::unlockOutpoint(const Wire::OutPoint& outpoint)
{
    _lockedOutpoints.erase(outpoint);
}

//==============================================================================

void RegtestChain::disconnectTip()
{
    auto tipBlock = _blocks.back();
    auto blockHash = GetHashStr(tipBlock.header);

    for (size_t j = 0; j < tipBlock.transactions.size(); ++j) {
        auto tx = tipBlock.transactions.at(j);
        auto txid = GetHash(tx);
        for (uint32_t i = 0; i < tx.txOut.size(); ++i) {
            _unspentUTXOs.erase(Wire::OutPoint{ txid, i });
        }

        if (j > 0) {
            for (auto&& in : tx.txIn) {
                if (auto tx = transactionByTxId(QString::fromStdString(txid.ToString()))) {
                    _unspentUTXOs.emplace(
                        in.previousOutPoint, tx.get().txOut.at(in.previousOutPoint.index));
                } else {
                    Q_ASSERT(false);
                }
            }
        }

        _transactionIndex.erase(QString::fromStdString(txid.ToString()));
    }

    _blocks.resize(_blocks.size() - 1);
    _blocksIndexByHash.erase(blockHash);
    _disconnectedHeaders.insert(blockHash, tipBlock.header);

    LogCDebug(General) << "Disconnecting tip:" << QString::fromStdString(blockHash)
                       << "new tip:" << GetHashStr(tip()).c_str() << height();
    tipDisconnected();
}

//==============================================================================

RegtestChain::RegtestChain(AssetID assetID, const WalletAssetsModel& assetsModel, QObject* parent)
    : QObject(parent)
    , _assetID(assetID)
    , _assetsModel(assetsModel)
    , _disconnectedHeaders(GetMaxReorgDepth(true))
{
}

//==============================================================================

void RegtestChain::load()
{
    if (_assetID == 0) {
        processNewBlock(MakeBlock(MakeHeader(1, "0x0",
            "0x4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b", 1296688602,
            0x207fffff, 2)));

        Q_ASSERT(GetHash(tip())
            == uint256S("0x0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206"));
    } else if (_assetID == 384) {
        processNewBlock(MakeBlock(MakeHeader(1, "0x0",
            "0x922ab2360f766457416dfc59c6594248c5b79e33c8785bce491c0e01930738f6", 1417713337,
            0x207fffff, 1)));

        //        Q_ASSERT(GetHash(tip()) ==
        //        uint256S("0x7229c32afcd2400f71cd72f55955c73bd384e41ed2e290c5a0488d0e6a15ddb4"));
    } else if (_assetID == 2) {
        processNewBlock(MakeBlock(MakeHeader(1, "0x0",
            "0x97ddfbbae6be97fd6cdf3e7ca13232a3afff2353e29badfab7f73011edd4ced9", 1296688602,
            0x207fffff, 0)));

        Q_ASSERT(GetHash(tip())
            == uint256S("0x530827f38f93b43ed12af0b3ad25a288dc02ed74d6d7857862df51fc56c416f9"));
    }
}

//==============================================================================

size_t RegtestChain::height() const
{
    return _blocks.size();
}

//==============================================================================

std::string RegtestChain::bestBlockHash() const
{
    return GetHashStr(_blocks.back().header);
}

//==============================================================================

Wire::MsgBlock RegtestChain::blockAt(size_t index) const
{
    return _blocks.at(index);
}

//==============================================================================

Wire::MsgBlock RegtestChain::blockByHash(BlockHash hash) const
{
    auto index = indexOf(hash);
    if (index >= 0) {
        return blockAt(index);
    }

    if (auto header = _disconnectedHeaders.get(hash.toStdString())) {
        Wire::MsgBlock block;
        block.header = header.get();
        return block;
    }

    throw std::runtime_error("No header found");
}

//==============================================================================

int32_t RegtestChain::indexOf(BlockHash hash) const
{
    return _blocksIndexByHash.count(hash.toStdString()) > 0
        ? _blocksIndexByHash.at(hash.toStdString())
        : -1;
}

//==============================================================================

const Wire::VerboseBlockHeader::Header& RegtestChain::tip() const
{
    return _blocks.back().header;
}

//==============================================================================

boost::optional<Wire::MsgTx> RegtestChain::transactionByTxId(QString txid) const
{
    if (_transactionIndex.count(txid) > 0) {
        const auto& block = blockByHash(_transactionIndex.at(txid));

        auto it = std::find_if(std::begin(block.transactions), std::end(block.transactions),
            [txid](const Wire::MsgTx& tx) {
                return ConvertToBitcoinTx(tx).GetHash().ToString() == txid.toStdString();
            });

        if (it != std::end(block.transactions)) {
            return *it;
        }
    }

    return boost::none;
}

//==============================================================================

bitcoin::uint256 GetHash(const Wire::VerboseBlockHeader::Header& header)
{
    bitcoin::CHashWriter ss(bitcoin::SER_GETHASH, bitcoin::PROTOCOL_VERSION);
    ss << header.version << bitcoin::uint256S(header.prevBlock)
       << bitcoin::uint256S(header.merkleRoot) << header.timestamp << header.bits << header.nonce;

    return ss.GetHash();
}

//==============================================================================

Wire::VerboseBlockHeader::Header MakeHeader(int32_t version, std::string prevBlock,
    std::string merkleRoot, uint32_t timestamp, uint32_t bits, uint32_t nonce)
{
    return Wire::VerboseBlockHeader::Header{ version, prevBlock, merkleRoot, timestamp, bits,
        nonce };
}

//==============================================================================

Wire::MsgBlock MakeBlock(
    Wire::VerboseBlockHeader::Header header, std::vector<Wire::MsgTx> transactions)
{
    return Wire::MsgBlock{ header, transactions };
}

//==============================================================================

BlockAssembler::BlockAssembler(const RegtestChain& chain)
    : _chain(chain)
{
}

//==============================================================================

Wire::MsgBlock BlockAssembler::CreateNewBlock(const CScript& coinbase_script)
{
    auto header = MakeHeader(2, _chain.bestBlockHash(), std::string(), GetTimeMillis(),
        CalculateNextWorkRequired(_chain.tip()), 0);

    Wire::MsgTx coinbaseTx;

    coinbaseTx.version = 1;
    coinbaseTx.txIn.resize(1);
    coinbaseTx.txOut.resize(1);

    coinbaseTx.txOut[0].pkScript
        = std::vector<unsigned char>(coinbase_script.begin(), coinbase_script.end());
    coinbaseTx.txOut[0].value = 50 * bitcoin::COIN;
    auto sigScript = CScript() << (_chain.height() + 1) << OP_0;
    coinbaseTx.txIn[0].signatureScript
        = std::vector<unsigned char>(sigScript.begin(), sigScript.end());
    //    pblocktemplate->vchCoinbaseCommitment = GenerateCoinbaseCommitment(*pblock, pindexPrev,
    //    chainparams.GetConsensus());
    std::vector<Wire::MsgTx> txns;
    txns.push_back(coinbaseTx);
    return MakeBlock(header, txns);
}

//==============================================================================

RegtestDataSource::RegtestDataSource(const WalletAssetsModel& assetsModel, QObject* parent)
    : AbstractChainDataSource(parent)
    , _assetsModel(assetsModel)
{
    auto loadChain = [this](AssetID assetID) {
        std::unique_ptr<RegtestChain> chain(new RegtestChain(assetID, _assetsModel, this));
        chain->load();
        chain->generateBlocks({}, 5);
        _chains.emplace(assetID, chain.release());
    };
    loadChain(0);
    loadChain(2);
    loadChain(384);
}

//==============================================================================

Promise<std::vector<Wire::VerboseBlockHeader>> RegtestDataSource::getBlockHeaders(
    AssetID assetID, BlockHash startingBlockHash) const
{
    return Promise<std::vector<Wire::VerboseBlockHeader>>(
        [=](const auto& resolve, const auto& reject) {
            const auto& currentChain = this->chain(assetID);
            auto height = currentChain.indexOf(startingBlockHash);
            if (height < 0) {
                NetworkUtils::ApiErrorException error(400, QString());
                error.errors.emplace_back("field-validation", "blockhash", "Block not found");
                reject(error);
            } else {
                std::vector<Wire::VerboseBlockHeader> result;
                for (size_t i = 1; i < 10; ++i) {
                    auto h = height + i;
                    if (h < currentChain.height()) {
                        Wire::VerboseBlockHeader header;
                        header.header = currentChain.blockAt(h).header;
                        header.height = h;
                        header.hash = GetHashStr(header.header);
                        result.push_back(header);
                    } else {
                        break;
                    }
                }

                resolve(result);
            }
        });
}

//==============================================================================

Promise<Wire::VerboseBlockHeader> RegtestDataSource::getBlockHeader(
    AssetID assetID, BlockHash hash) const
{
    return Promise<Wire::VerboseBlockHeader>([=](const auto& resolve, const auto& reject) {
        try {
            auto height = this->chain(assetID).indexOf(hash);
            Wire::VerboseBlockHeader verboseHeader;
            verboseHeader.header = this->chain(assetID).blockByHash(hash).header;
            verboseHeader.hash = hash.toStdString();
            verboseHeader.height = height;
            resolve(verboseHeader);
        } catch (std::exception& ex) {
            reject(ex);
        }
    });
}

//==============================================================================

Promise<BlockHash> RegtestDataSource::getBlockHash(AssetID assetID, size_t blockHeight) const
{
    return QtPromise::resolve(
        QString::fromStdString(GetHashStr(chain(assetID).blockAt(blockHeight).header)));
}

//==============================================================================

Promise<std::vector<unsigned char>> RegtestDataSource::getBlock(
    AssetID assetID, BlockHash hash) const
{
    //    return getFilteredBlock(assetID, hash).then([=](std::vector<std::string> transactions) {
    //        Wire::MsgEncodedBlock block;
    //        block.header = chain(assetID).blockByHash(hash).header;
    //        block.transactions = transactions;
    //        return block;
    //    });
    return QtPromise::resolve(std::vector<unsigned char>{});
}

//==============================================================================

Promise<std::vector<std::string>> RegtestDataSource::getFilteredBlock(
    AssetID assetID, BlockHash hash) const
{
    std::vector<std::string> txns;
    const auto& block = chain(assetID).blockByHash(hash);
    for (auto&& tx : block.transactions) {
        auto btcTx = ConvertToBitcoinTx(tx);
        CDataStream ss(SER_NETWORK, bitcoin::PROTOCOL_VERSION);
        ss << btcTx;
        txns.push_back(bitcoin::HexStr(ss));
    }
    return QtPromise::resolve(txns);
}

//==============================================================================

Promise<Wire::EncodedBlockFilter> RegtestDataSource::getBlockFilter(
    AssetID assetID, BlockHash hash) const
{
    uint256 h = uint256S(hash.toStdString());
    Filter::key_t key;

    std::memcpy(&key[0], h.begin(), key.size());

    auto filter = Filter::WithKeyMP(key, 784931, 19);

    const auto& currentChain = chain(assetID);
    auto block = currentChain.blockByHash(hash);
    std::vector<std::string> entries;
    for (size_t j = 0; j < block.transactions.size(); ++j) {
        auto tx = block.transactions.at(j);
        if (j > 0) // skip coinbase
        {
            for (auto& in : tx.txIn) {
                if (auto tx = currentChain.transactionByTxId(
                        QString::fromStdString(in.previousOutPoint.hash.ToString()))) {
                    filter.addEntry(tx->txOut.at(in.previousOutPoint.index).pkScript);
                }
            }
        }

        for (auto out : tx.txOut) {
            filter.addEntry(out.pkScript);
        }
    }

    filter.addEntries(entries);
    const auto N = filter.build();

    return QtPromise::resolve(Wire::EncodedBlockFilter(N, 784931, 19, filter.bytes()));
}

//==============================================================================

Promise<boost::optional<Wire::TxOut>> RegtestDataSource::getUTXO(
    AssetID assetID, const Wire::OutPoint& outpoint) const
{
    auto it = chain(assetID).utxoSet().find(outpoint);
    boost::optional<Wire::TxOut> result;
    if (it != std::end(chain(assetID).utxoSet())) {
        result = it->second;
    }

    return QtPromise::resolve(result);
}

//==============================================================================

Promise<QString> RegtestDataSource::sendRawTransaction(AssetID assetID, QString serializedTx) const
{
    CDataStream ss(ParseHex(serializedTx.toStdString()), SER_NETWORK, bitcoin::PROTOCOL_VERSION);
    CMutableTransaction tx;
    ss >> tx;
    auto txid = QString::fromStdString(tx.GetHash().ToString());
    chain(assetID).mempool().push_back(ConvertToWireTx(tx));
    LogCDebug(General) << "Added to mempool tx:" << txid;
    return QtPromise::resolve(txid);
}

//==============================================================================

Promise<Wire::TxConfrimation> RegtestDataSource::getRawTransaction(
    AssetID assetID, QString txid) const
{
    Wire::TxConfrimation result;
    if (auto txOpt = chain(assetID).transactionByTxId(txid)) {
        auto btcTx = ConvertToBitcoinTx(*txOpt);
        CDataStream ss(SER_NETWORK, bitcoin::PROTOCOL_VERSION);
        ss << btcTx;
        result.hexTx = bitcoin::HexStr(ss);
        result.txIndex = 0;
        result.blockHeight = 0;
    }
    return QtPromise::resolve(result);
}

//==============================================================================

Promise<boost::optional<Wire::TxOut>> RegtestDataSource::getTxOut(
    AssetID, Wire::OutPoint outpoint) const
{
    return Promise<boost::optional<Wire::TxOut>>(
        [](const auto& resolve, const auto& reject) { reject(); });
}

//==============================================================================

RegtestChain& RegtestDataSource::chain(AssetID assetID) const
{
    return *_chains.at(assetID);
}

//==============================================================================

Promise<void> RegtestChainManager::loadChains(std::vector<AssetID> chains)
{
    auto setNewTip = [this](AssetID, const auto&) {

    };

    auto getHeader
        = [this](AssetID assetID, size_t height) -> boost::optional<Wire::VerboseBlockHeader> {
        auto block = _dataSource.chain(assetID).blockAt(height);
        Wire::VerboseBlockHeader header;
        header.header = block.header;
        header.height = height;
        header.hash = GetHashStr(block.header);
        return header;
    };

    std::vector<AssetID> assets = { 0, 2, 384 };

    for (auto&& assetID : assets) {
        auto set = std::bind(setNewTip, assetID, std::placeholders::_1);
        auto get = std::bind(getHeader, assetID, std::placeholders::_1);
        _chains.emplace(assetID, std::unique_ptr<Chain>(new Chain(assetID, set, get)));

        const auto& dt = _dataSource.chain(assetID);

        connect(&dt, &RegtestChain::tipConnected, this,
            [this, assetID](QString newBlockHash, int newHeight) {
                const auto& dt = _dataSource.chain(assetID);
                Wire::VerboseBlockHeader header;
                header.header = dt.blockAt(newHeight).header;
                header.height = newHeight;
                header.hash = newBlockHash.toStdString();
                connectTip(assetID, header);
            });

        connect(
            &dt, &RegtestChain::tipDisconnected, this, [this, assetID] { disconnectTip(assetID); });

        Wire::VerboseBlockHeader header;
        header.header = dt.tip();
        header.height = dt.height() - 1;
        header.hash = dt.bestBlockHash();

        connectTip(assetID, header);
    }

    chainsLoaded(assets);
    return QtPromise::resolve();
}

//==============================================================================

Promise<void> RegtestDataSource::loadSecondLayerCache(
    AssetID assetID, uint32_t startBlock, Interrupt* onInterrupt) const
{
    return QtPromise::resolve();
}

//==============================================================================

Promise<void> RegtestDataSource::freeSecondLayerCache(AssetID assetID) const
{
    return QtPromise::resolve();
}

//==============================================================================

Promise<std::string> RegtestDataSource::getRawTxByIndex(
    AssetID assetID, int64_t blockNum, uint32_t txIndex) const
{
    Wire::MsgBlock block = _chains.at(assetID)->blockAt(size_t(blockNum));
    auto btcTx = ConvertToBitcoinTx(block.transactions.at(txIndex));
    CDataStream ss(SER_NETWORK, bitcoin::PROTOCOL_VERSION);
    ss << btcTx;
    return QtPromise::resolve(bitcoin::HexStr(ss));
}

//==============================================================================

Promise<int64_t> RegtestDataSource::estimateNetworkFee(AssetID assetID, uint64_t blocks) const
{
    return QtPromise::resolve(static_cast<int64_t>(1));
}

//==============================================================================

Promise<Wire::TxConfrimation> RegtestDataSource::getSpendingDetails(
    AssetID assetID, Wire::OutPoint outpoint) const
{
    Wire::TxConfrimation result;
    return QtPromise::resolve(result);
}

//==============================================================================

Promise<void> RegtestChainManager::loadFromBootstrap(QString bootstrapFile)
{
    return QtPromise::resolve();
}

//==============================================================================

void RegtestChainManager::connectTip(AssetID assetID, Wire::VerboseBlockHeader newTip)
{
    _chains.at(assetID)->connectTip(newTip);
}

//==============================================================================

void RegtestChainManager::disconnectTip(AssetID assetID)
{
    _chains.at(assetID)->disconnectTip();
}

//==============================================================================

RegtestChainManager::RegtestChainManager(
    RegtestDataSource& regtestDataSource, const WalletAssetsModel& assetsModel)
    : AbstractMutableChainManager(assetsModel)
    , _dataSource(regtestDataSource)
{
}

//==============================================================================

uint256 GetHash(const Wire::MsgTx& tx)
{
    return ConvertToBitcoinTx(tx).GetHash();
}

//==============================================================================

Promise<std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>>> RegtestDataSource::listUTXOs(
    AssetID assetID) const
{
    std::vector<std::tuple<Wire::OutPoint, Wire::TxOut>> result;
    const auto& ch = chain(assetID);
    const auto& utxos = ch.utxoSet();

    for (auto&& it : utxos) {
        result.emplace_back(it.first, it.second);
    }

    return QtPromise::resolve(result);
}

//==============================================================================

Promise<void> RegtestDataSource::lockOutpoint(AssetID assetID, Wire::OutPoint outpoint)
{
    chain(assetID).lockOutpoint(outpoint);
    return QtPromise::resolve();
}

//==============================================================================

Promise<void> RegtestDataSource::unlockOutpoint(AssetID assetID, Wire::OutPoint outpoint)
{
    chain(assetID).unlockOutpoint(outpoint);
    return QtPromise::resolve();
}

//==============================================================================
Promise<void> RegtestDataSource::load()
{
    return QtPromise::resolve();
}

//==============================================================================

Promise<Wire::StrippedBlock> RegtestDataSource::getLightWalletBlock(
    AssetID assetID, BlockHash hash, int64_t blockHeight) const
{
    throw std::runtime_error("Not implemented");
}

//==============================================================================

Promise<std::tuple<BlockHash, BlockHeight>> RegtestDataSource::getBestBlockHash(
    AssetID assetID) const
{
    if (auto chain = _chains.at(assetID)) {
        return QtPromise::resolve(
            std::make_tuple(QString::fromStdString(chain->bestBlockHash()), chain->height() - 1));
    }

    throw std::runtime_error("Unexpected error");
}

//==============================================================================
