// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_TXDB_H
#define BITCOIN_TXDB_H

#include <dbwrapper.h>
#include <transaction.h>
#include <uint256.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace bitcoin {

class CBlockIndex;
class CCoinsViewDBCursor;
class uint256;

using AssetID = uint32_t;

//! No need to periodic flush if at least this much space still available.
static constexpr int MAX_BLOCK_COINSDB_USAGE = 10;
//! -dbcache default (MiB)
static const int64_t nDefaultDbCache = 450;
//! -dbbatchsize default (bytes)
static const int64_t nDefaultDbBatchSize = 16 << 20;
//! max. -dbcache (MiB)
static const int64_t nMaxDbCache = sizeof(void*) > 4 ? 16384 : 1024;
//! min. -dbcache (MiB)
static const int64_t nMinDbCache = 4;
//! Max memory allocated to block tree DB specific cache, if no -txindex (MiB)
static const int64_t nMaxBlockDBCache = 2;
//! Max memory allocated to block tree DB specific cache, if -txindex (MiB)
// Unlike for the UTXO database, for the txindex scenario the leveldb cache make
// a meaningful difference: https://github.com/bitcoin/bitcoin/pull/8273#issuecomment-229601991
static const int64_t nMaxTxIndexCache = 1024;
//! Max memory allocated to coin DB specific cache (MiB)
static const int64_t nMaxCoinsDBCache = 8;

class CBlockFileInfo {
public:
    unsigned int nBlocks; //!< number of blocks stored in file
    unsigned int nSize; //!< number of used bytes of block file
    unsigned int nUndoSize; //!< number of used bytes in the undo file
    unsigned int nHeightFirst; //!< lowest height of block in file
    unsigned int nHeightLast; //!< highest height of block in file
    uint64_t nTimeFirst; //!< earliest time of block in file
    uint64_t nTimeLast; //!< latest time of block in file

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(VARINT(nBlocks));
        READWRITE(VARINT(nSize));
        READWRITE(VARINT(nUndoSize));
        READWRITE(VARINT(nHeightFirst));
        READWRITE(VARINT(nHeightLast));
        READWRITE(VARINT(nTimeFirst));
        READWRITE(VARINT(nTimeLast));
    }

    void SetNull()
    {
        nBlocks = 0;
        nSize = 0;
        nUndoSize = 0;
        nHeightFirst = 0;
        nHeightLast = 0;
        nTimeFirst = 0;
        nTimeLast = 0;
    }

    CBlockFileInfo()
    {
        SetNull();
    }

    std::string ToString() const;

    /** update statistics (does not update nSize) */
    void AddBlock(unsigned int nHeightIn, uint64_t nTimeIn)
    {
        if (nBlocks == 0 || nHeightFirst > nHeightIn)
            nHeightFirst = nHeightIn;
        if (nBlocks == 0 || nTimeFirst > nTimeIn)
            nTimeFirst = nTimeIn;
        nBlocks++;
        if (nHeightIn > nHeightLast)
            nHeightLast = nHeightIn;
        if (nTimeIn > nTimeLast)
            nTimeLast = nTimeIn;
    }
};

class CDiskBlockFilter {
public:
    uint32_t n;
    uint64_t m;
    uint16_t p;
    std::vector<unsigned char> vchBlockFilter;

    CDiskBlockFilter() = default;

    CDiskBlockFilter(uint32_t nIn, uint64_t mIn, uint16_t pIn, std::vector<unsigned char> blockFilter)
        : n(nIn)
        , m(mIn)
        , p(pIn)
        , vchBlockFilter(blockFilter)
    {
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(n);
        READWRITE(m);
        READWRITE(p);
        READWRITE(vchBlockFilter);
    }
};

/** Used to marshal pointers into hashes for db storage. */
class CDiskBlockIndex {
public:
    int32_t nVersion;
    uint256 hashPrevBlock;
    uint256 hashMerkleRoot;
    uint32_t nTimestamp;
    uint32_t nBits;
    uint32_t nNonce;

    uint32_t nHeight;
    uint256 hashBlock;
    CDiskBlockFilter blockFilter;

    CDiskBlockIndex() = default;

    CDiskBlockIndex(int32_t nVersionIn, uint256 hashPrevBlockIn, uint256 hashMerkleRootIn,
        uint32_t nTimestampIn, uint32_t nBitsIn, uint32_t nNonceIn,
        uint32_t nHeightIn, uint256 hashBlockIn, CDiskBlockFilter blockFilterIn)
        : nVersion(nVersionIn)
        , hashPrevBlock(hashPrevBlockIn)
        , hashMerkleRoot(hashMerkleRootIn)
        , nTimestamp(nTimestampIn)
        , nBits(nBitsIn)
        , nNonce(nNonceIn)
        , nHeight(nHeightIn)
        , hashBlock(hashBlockIn)
        , blockFilter(blockFilterIn)
    {
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        // block header
        READWRITE(nVersion);
        READWRITE(hashPrevBlock);
        READWRITE(hashMerkleRoot);
        READWRITE(nTimestamp);
        READWRITE(nBits);
        READWRITE(nNonce);

        // verbose info
        READWRITE(nHeight);
        READWRITE(hashBlock);
        READWRITE(blockFilter);
    }

    uint256 GetBlockHash() const
    {
        return hashBlock;
    }
};

/** Access to the block database (blocks/index/) */
class CBlockTreeDB : public CDBWrapper {
public:
    explicit CBlockTreeDB(std::string dataDir, size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    //    void WriteBestBlockBatch(CDBBatch &batch, AssetID assetId, uint256 hashBestBlock);
    //    bool WriteBestBlockSync(AssetID assetId, std::pair<uint256, uint64_t> &bestBlock);

    //    bool WriteBatchSync(const std::vector<std::pair<AssetID, CDiskBlockIndex>> &blockinfo);
    //    bool WriteBatchSync(CDBBatch &batch, const std::vector<std::pair<AssetID, CDiskBlockIndex>> &blockinfo);

    void WriteBestBlock(CDBBatch& batch, AssetID assetID, uint32_t bestBlockHeight);
    bool ReadBestBlock(AssetID assetID, uint32_t& bestBlockHeight);
    void WriteBlockIndex(CDBBatch& batch, AssetID assetID, CDiskBlockIndex blockIndex);

    bool ReadBlockIndex(AssetID assetID, uint32_t height, CDiskBlockIndex& blockIndex);

    void EraseBlockIndex(CDBBatch& batch, AssetID assetID, uint32_t height);

    //    bool ReadBlockFileInfo(int nFile, CBlockFileInfo &info);
    //    bool ReadLastBlockFile(int &nFile);
    //    bool WriteReindexing(bool fReindexing);
    //    void ReadReindexing(bool &fReindexing);
    //    bool WriteFlag(const std::string &name, bool fValue);
    //    bool ReadFlag(const std::string &name, bool &fValue);

    bool LoadBlockIndexGuts(std::function<void(AssetID, const CDiskBlockIndex&)> loadedBlockIndex);
    bool LoadBestBlockGuts(std::function<void(AssetID, uint32_t)> loadedBlock);
    bool LoadBestBlockGuts(std::function<void(AssetID, uint32_t)> loadedBlock, std::vector<AssetID> assets);
    bool EraseByAsset(unsigned assetID);
    bool Exists(unsigned assetID);
};

class CDiskTxOut {
public:
    int outputIndex;
    std::string address;
    CAmount value;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->outputIndex);
        READWRITE(this->address);
        READWRITE(this->value);
    }

    CDiskTxOut()
    {
        SetNull();
    }

    CDiskTxOut(int outIndex, std::string addr, CAmount val)
        : outputIndex(outIndex)
        , address(addr)
        , value(val)
    {
    }

    void SetNull()
    {
        outputIndex = -1;
        address.clear();
        value = 0;
    }

    bool operator==(const CDiskTxOut& rhs) const
    {
        return outputIndex == rhs.outputIndex && value == rhs.value && address == rhs.address;
    }

    bool IsValid() const
    {
        return outputIndex >= 0 && !address.empty() && value >= 0;
    }
};

class CDiskTxIn {
public:
    uint256 hash;
    int32_t outputIndex;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->hash);
        READWRITE(this->outputIndex);
    }

    CDiskTxIn()
    {
        SetNull();
    }

    CDiskTxIn(uint256 h, int32_t outIndex)
        : hash(h)
        , outputIndex(outIndex)
    {
    }

    void SetNull()
    {
        hash.SetNull();
        outputIndex = -1;
    }

    bool operator==(const CDiskTxIn& rhs) const
    {
        return hash == rhs.hash && outputIndex == rhs.outputIndex;
    }

    bool IsValid() const
    {
        return !hash.IsNull() && outputIndex >= 0;
    }
};

class CDiskTx {
public:
    AssetID assetID;
    uint256 transactionID;
    uint256 blockHash;
    int64_t blockHeight;
    uint32_t transactionIndex;
    int64_t transactionDate;
    std::vector<unsigned char> vchSerialization;
    uint32_t flags;

    std::vector<CDiskTxIn> vIn;
    std::vector<CDiskTxOut> vOut;

    CDiskTx()
    {
        SetNull();
    }

    CDiskTx(AssetID assetId,
        uint256 txId,
        uint256 blockHashIn,
        int64_t blockHeightIn,
        uint32_t transactionIndexIn,
        int64_t txDate,
        uint32_t txFlags)
        : assetID(assetId)
        , transactionID(txId)
        , blockHash(blockHashIn)
        , blockHeight(blockHeightIn)
        , transactionIndex(transactionIndexIn)
        , transactionDate(txDate)
        , flags(txFlags)
    {
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->assetID);
        READWRITE(this->transactionID);
        READWRITE(this->blockHash);
        READWRITE(this->blockHeight);
        READWRITE(this->transactionIndex);
        READWRITE(this->transactionDate);
        READWRITE(this->vIn);
        READWRITE(this->vOut);
        READWRITE(this->vchSerialization);
        READWRITE(this->flags);
    }

    bool IsValid() const
    {
        return !transactionID.IsNull() && transactionDate > 0;
    }

    void SetNull()
    {
        assetID = 0;
        transactionID.SetNull();
        blockHash.SetNull();
        blockHeight = 0;
        transactionIndex = 0;
        transactionDate = 0;
        flags = 0;
        vchSerialization.clear();
    }
};

using TransactionDBList = std::vector<CDiskTx>;
using TransactionDBMap = std::map<AssetID, TransactionDBList>;

/** Access to the block database (blocks/index/) */
class CTxDB : public CDBWrapper {
public:
    explicit CTxDB(std::string dataDir, size_t nCacheSize, bool fMemory = false, bool fWipe = false);

    bool WriteBatchSync(std::vector<CDiskTx> transactions);
    bool WriteBatchSync(CDBBatch& batch, std::vector<CDiskTx> transactions);
    bool LoadTxDBGuts(std::function<void(const CDiskTx&)> loadedTransaction);
};
}

#endif // BITCOIN_TXDB_H
