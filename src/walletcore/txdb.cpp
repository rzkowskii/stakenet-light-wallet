// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <txdb.h>

#include <hash.h>
#include <random.h>
#include <uint256.h>

#include <QDebug>
#include <boost/thread.hpp>
#include <stdint.h>

static const char DB_BLOCK_FILES = 'f';
static const char DB_BLOCK_INDEX = 'b';
static const char DB_TX_INDEX = 't';
static const std::string DB_BLOCK_TX_INDEX = "btx";

static const char DB_BEST_BLOCK = 'B';
static const char DB_HEAD_BLOCKS = 'H';
static const char DB_FLAG = 'F';
static const char DB_REINDEX_FLAG = 'R';
static const char DB_LAST_BLOCK = 'l';
static const char DB_TIP_HASH = 'T';

namespace bitcoin {

CBlockTreeDB::CBlockTreeDB(std::string dataDir, size_t nCacheSize, bool fMemory, bool fWipe)
    : CDBWrapper(dataDir, nCacheSize, fMemory, fWipe)
{
}

void CBlockTreeDB::WriteBestBlock(CDBBatch& batch, AssetID assetID, uint32_t bestBlockHeight)
{
    batch.Write(std::make_pair(DB_TIP_HASH, assetID), bestBlockHeight);
}

bool CBlockTreeDB::ReadBestBlock(AssetID assetID, uint32_t& bestBlockHeight)
{
    return Read(std::make_pair(DB_TIP_HASH, assetID), bestBlockHeight);
}

void CBlockTreeDB::WriteBlockIndex(CDBBatch& batch, AssetID assetID, CDiskBlockIndex blockIndex)
{
    batch.Write(
        std::make_pair(DB_BLOCK_INDEX, std::make_pair(assetID, blockIndex.nHeight)), blockIndex);
}

bool CBlockTreeDB::ReadBlockIndex(AssetID assetID, uint32_t height, CDiskBlockIndex& blockIndex)
{
    return Read(std::make_pair(DB_BLOCK_INDEX, std::make_pair(assetID, height)), blockIndex);
}

void CBlockTreeDB::EraseBlockIndex(CDBBatch& batch, AssetID assetID, uint32_t height)
{
    batch.Erase(std::make_pair(DB_BLOCK_INDEX, std::make_pair(assetID, height)));
}

// void CBlockTreeDB::WriteBestBlockBatch(CDBBatch &batch, AssetID assetId, uint256 hashBestBlock)
//{
//    batch.Write(std::make_pair(DB_TIP_HASH, assetId), hashBestBlock);
//}

// bool CBlockTreeDB::WriteBestBlockSync(AssetID assetId, std::pair<uint256, uint64_t> &bestBlock)
//{
//    CDBBatch batch(*this);
//    batch.Write(std::make_pair(DB_TIP_HASH, assetId), bestBlock);
//    return WriteBatch(batch, true);
//}

// bool CBlockTreeDB::WriteBatchSync(const std::vector<std::pair<AssetID, CDiskBlockIndex> >
// &blockinfo)
//{
//    CDBBatch batch(*this);
//    return WriteBatchSync(batch, blockinfo);
//}

// bool CBlockTreeDB::WriteBatchSync(CDBBatch &batch, const std::vector<std::pair<AssetID,
// CDiskBlockIndex> > &blockinfo)
//{
//    for (auto &&info : blockinfo) {
//        batch.Write(std::make_pair(DB_BLOCK_INDEX, std::make_pair(info.first,
//        info.second.GetBlockHash())), info.second);
//    }
//    return WriteBatch(batch, true);
//}

// bool CBlockTreeDB::ReadBestBlock(AssetID assetId, std::pair<uint256, uint64_t> &bestBlock)
//{
//    return Read(std::make_pair(DB_TIP_HASH, assetId), bestBlock);
//}

// bool CBlockTreeDB::ReadBlockIndex(AssetID assetId, const uint256 &hashBlock, CDiskBlockIndex
// &blockIndex)
//{
//    return Read(std::make_pair(DB_BLOCK_INDEX, std::make_pair(assetId, hashBlock)), blockIndex);
//}

// bool CBlockTreeDB::WriteFlag(const std::string &name, bool fValue) {
//    return Write(std::make_pair(DB_FLAG, name), fValue ? '1' : '0');
//}

// bool CBlockTreeDB::ReadFlag(const std::string &name, bool &fValue) {
//    char ch;
//    if (!Read(std::make_pair(DB_FLAG, name), ch))
//        return false;
//    fValue = ch == '1';
//    return true;
//}

bool CBlockTreeDB::LoadBlockIndexGuts(
    std::function<void(AssetID, const CDiskBlockIndex&)> loadedBlockIndex)
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());
    std::pair<char, std::pair<AssetID, uint32_t>> key; // { DB_BLOCK_INDEX, { 384 , 0 }};
    pcursor->Seek(DB_BLOCK_INDEX);

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        if (pcursor->GetKey(key) && key.first == DB_BLOCK_INDEX) {
            CDiskBlockIndex diskindex;
            if (pcursor->GetValue(diskindex)) {
                // Construct block index object
                loadedBlockIndex(key.second.first, diskindex);
                pcursor->Next();
            } else {
                return false;
                //                return error("%s: failed to read value", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::LoadBestBlockGuts(std::function<void(AssetID, uint32_t)> loadedBlock)
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(DB_TIP_HASH);

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, AssetID> key;
        if (pcursor->GetKey(key) && key.first == DB_TIP_HASH) {
            uint32_t bestHeight = 0;
            if (pcursor->GetValue(bestHeight)) {
                // Construct block index object
                loadedBlock(key.second, bestHeight);
                pcursor->Next();
            } else {
                return false;
                //                return error("%s: failed to read value", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}

bool CBlockTreeDB::LoadBestBlockGuts(
    std::function<void(AssetID, uint32_t)> loadedBlock, std::vector<AssetID> assets)
{
    for (auto&& assetID : assets) {
        uint32_t bestHeight = 0;
        if (Read(std::make_pair(DB_TIP_HASH, assetID), bestHeight)) {
            loadedBlock(assetID, bestHeight);
        }
    }

    return true;
}

bool CBlockTreeDB::EraseByAsset(unsigned assetID)
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(DB_TX_INDEX, uint256()));

    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, std::pair<AssetID, uint256>> key;
        if (pcursor->GetKey(key) && key.first == DB_TX_INDEX && key.second.first == assetID) {
            Erase(key);
        }
    }

    return true;
}

bool CBlockTreeDB::Exists(unsigned assetID)
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(DB_TX_INDEX, uint256()));
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, std::pair<AssetID, uint256>> key;
        if (pcursor->GetKey(key) && key.first == DB_TX_INDEX && key.second.first == assetID) {
            return true;
        }
    }
    return false;
}

CTxDB::CTxDB(std::string dataDir, size_t nCacheSize, bool fMemory, bool fWipe)
    : CDBWrapper(dataDir, nCacheSize, fMemory, fWipe)
{
}

bool CTxDB::WriteBatchSync(std::vector<CDiskTx> transactions)
{
    CDBBatch batch(*this);
    WriteBatchSync(batch, transactions);
    return WriteBatch(batch, true);
}

bool CTxDB::WriteBatchSync(CDBBatch& batch, std::vector<CDiskTx> transactions)
{
    for (auto&& transaction : transactions) {
        batch.Write(std::make_pair(DB_TX_INDEX,
                        std::make_pair(transaction.assetID, transaction.transactionID)),
            transaction);
    }

    return WriteBatch(batch, true);
}

bool CTxDB::LoadTxDBGuts(std::function<void(const CDiskTx&)> loadedTransaction)
{
    std::unique_ptr<CDBIterator> pcursor(NewIterator());

    pcursor->Seek(std::make_pair(DB_TX_INDEX, uint256()));

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        std::pair<char, std::pair<AssetID, uint256>> key;
        if (pcursor->GetKey(key) && key.first == DB_TX_INDEX) {
            CDiskTx diskTx;
            if (pcursor->GetValue(diskTx)) {
                // Construct block index object
                loadedTransaction(diskTx);
                pcursor->Next();
            } else {
                return false;
                //                return error("%s: failed to read value", __func__);
            }
        } else {
            break;
        }
    }

    return true;
}
}
