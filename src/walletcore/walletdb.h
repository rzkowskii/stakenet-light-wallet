// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_WALLETDB_H
#define BITCOIN_WALLET_WALLETDB_H

#include "hdchain.h"
#include "key.h"

#include "db.h"
#include <list>
#include <map>
#include <stdint.h>
#include <string>
#include <transaction.h>
#include <utility>
#include <vector>

namespace bitcoin {

static const bool DEFAULT_FLUSHWALLET = true;
/** Backend-agnostic database type. */
using WalletDatabase = BerkeleyDatabase;

struct CBlockLocator;
class CMasterKey;
class CScript;
class CWallet;
class uint160;
class uint256;

/** Error statuses for the wallet database */
enum DBErrors {
    DB_LOAD_OK,
    DB_CORRUPT,
    DB_NONCRITICAL_ERROR,
    DB_TOO_NEW,
    DB_LOAD_FAIL,
    DB_NEED_REWRITE
};

/** A key pool entry */
class CKeyPool {
public:
    CKeyID keyID;
    int64_t nTime;
    uint32_t nAccountIndex;
    uint32_t nHDKeyIndex;
    bool fInternal; // for change outputs

    CKeyPool() = default;
    CKeyPool(CKeyID keyIDIn, uint32_t accountIndex, uint32_t hdKeyIndex, bool internalIn)
        : keyID(keyIDIn)
        , nAccountIndex(accountIndex)
        , nHDKeyIndex(hdKeyIndex)
        , fInternal(internalIn)
    {
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(nTime);
        READWRITE(nAccountIndex);
        READWRITE(nHDKeyIndex);
        READWRITE(keyID);
        READWRITE(fInternal);
    }

    bool operator<(const CKeyPool& rhs) const
    {
        return nAccountIndex == rhs.nAccountIndex ? nHDKeyIndex < rhs.nHDKeyIndex : nAccountIndex < rhs.nAccountIndex;
    }
};

// set of keypools
using SetKeyPool = std::set<CKeyPool>;
struct KeyPools {
    SetKeyPool internalKeyPool;
    SetKeyPool externalKeyPool;

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(internalKeyPool);
        READWRITE(externalKeyPool);
    }
};

class CKeyMetadata {
public:
    static const int CURRENT_VERSION = 1;
    int nVersion;
    int64_t nCreateTime; // 0 means unknown

    CKeyMetadata()
    {
        SetNull();
    }
    CKeyMetadata(int64_t nCreateTime_)
    {
        SetNull();
        nCreateTime = nCreateTime_;
    }

    ADD_SERIALIZE_METHODS;

    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->nVersion);
        READWRITE(nCreateTime);
    }

    void SetNull()
    {
        nVersion = CKeyMetadata::CURRENT_VERSION;
        nCreateTime = 0;
    }
};

using AssetID = uint32_t;

// account index to set of keypools
using AccountSetKeyPool = std::map<uint32_t, KeyPools>;
// asset id to accounts
using AssetsSetKeyPools = std::map<AssetID, AccountSetKeyPool>;


/** Access to the wallet database.
 * Opens the database and provides read and write access to it. Each read and write is its own transaction.
 * Multiple operation transactions can be started using TxnBegin() and committed using TxnCommit()
 * Otherwise the transaction will be committed when the object goes out of scope.
 * Optionally (on by default) it will flush to disk on close.
 * Every 1000 writes will automatically trigger a flush to disk.
 */
class WalletBatch {
private:
    template <typename K, typename T>
    bool WriteIC(const K& key, const T& value, bool fOverwrite = true)
    {
        if (!m_batch.Write(key, value, fOverwrite)) {
            return false;
        }
        m_database.IncrementUpdateCounter();
        if (m_database.nUpdateCounter % 1000 == 0) {
            m_batch.Flush();
        }
        return true;
    }

    template <typename K>
    bool EraseIC(const K& key)
    {
        if (!m_batch.Erase(key)) {
            return false;
        }
        m_database.IncrementUpdateCounter();
        if (m_database.nUpdateCounter % 1000 == 0) {
            m_batch.Flush();
        }
        return true;
    }

public:
    explicit WalletBatch(WalletDatabase& database, const char* pszMode = "r+", bool _fFlushOnClose = true)
        : m_batch(database, pszMode, _fFlushOnClose)
        , m_database(database)
    {
    }

    bool WriteName(AssetID assetID, const std::string& strAddress, const std::string& strName);
    bool EraseName(AssetID assetID, const std::string& strAddress);

    bool WritePurpose(AssetID assetID, const std::string& strAddress, const std::string& purpose);
    bool ErasePurpose(AssetID assetID, const std::string& strAddress);

    bool HasMasterKey();
    bool WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey);

    bool WritePool(uint32_t nPurpose, AssetID assetID, const AccountSetKeyPool& pools);
    bool ErasePool(uint32_t nPurpose, AssetID assetID);

    bool WriteCScript(AssetID assetID, const uint160& hash, const CScript& redeemScript);

    bool WriteDefaultKey(const CPubKey& vchPubKey);

    bool WriteMinVersion(int nVersion);

    /// Write destination data key,value tuple to database
    bool WriteDestData(const std::string& address, const std::string& key, const std::string& value);
    /// Erase destination data tuple from wallet database
    bool EraseDestData(const std::string& address, const std::string& key);

    bool WriteAddressType(const std::map<AssetID, std::string> addressType);

    DBErrors LoadWallet(CWallet* pwallet);

    //! write the hdchain model (external chain child index counter)
    bool WriteHDChain(const CHDChain& chain);
    bool WriteCryptedHDChain(const CHDChain& chain);
    bool WriteHDPubKey(AssetID assetID, const CHDPubKey& hdPubKey, const CKeyMetadata& keyMeta);

    /* verifies the database environment */
    static bool VerifyEnvironment(const fs::path& wallet_path, std::string& errorStr);
    /* verifies the database file */
    static bool VerifyDatabaseFile(const fs::path& wallet_path, std::string& warningStr, std::string& errorStr);

    //! Begin a new transaction
    bool TxnBegin();
    //! Commit current transaction
    bool TxnCommit();
    //! Abort current transaction
    bool TxnAbort();

private:
    BerkeleyBatch m_batch;
    WalletDatabase& m_database;
};

void ThreadFlushWalletDB();
}

#endif // BITCOIN_WALLET_WALLETDB_H
