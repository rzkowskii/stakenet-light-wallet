// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "walletdb.h"

#include <atomic>
#include <base58.h>
#include <script/script.h>
#include <serialize.h>
#include <sync.h>
#include <tinyformat.h>
#include <utiltime.h>
#include <wallet.h>
#include <walletdb.h>

#include <boost/filesystem.hpp>
#include <boost/foreach.hpp>
#include <boost/thread.hpp>

#define LogPrintf

namespace bitcoin {

namespace keys {
    static const std::string VERSION{ "version" };
    static const std::string SCOPED_HD_CHAIN{ "scopedhdchain" };
    static const std::string CRYPTED_SCOPED_HD_CHAIN{ "cscopedhdchain" };

    namespace legacy {
        static const std::string MAIN_KEY_POOL{ "pool" };
        static const std::string AUX_KEY_POOL{ "auxaccountpool" };
        static const std::string AUX_HDCHAIN{ "auxhdchain" };
        static const std::string MAIN_HDCHAIN{ "hdchain" };
    }
}

static uint64_t nAccountingEntryNumber = 0;
static std::atomic<unsigned int> nWalletDBUpdateCounter;

//
// CWalletDB
//

bool WalletBatch::WriteName(
    AssetID assetID, const std::string& strAddress, const std::string& strName)
{
    return WriteIC(make_pair(std::string("name"), std::make_pair(assetID, strAddress)), strName);
}

bool WalletBatch::EraseName(AssetID assetID, const std::string& strAddress)
{
    // This should only be used for sending addresses, never for receiving addresses,
    // receiving addresses must always have an address book entry if they're not change return.
    return EraseIC(make_pair(std::string("name"), std::make_pair(assetID, strAddress)));
}

bool WalletBatch::WritePurpose(
    AssetID assetID, const std::string& strAddress, const std::string& strPurpose)
{
    return WriteIC(
        make_pair(std::string("purpose"), std::make_pair(assetID, strAddress)), strPurpose);
}

bool WalletBatch::ErasePurpose(AssetID assetID, const std::string& strAddress)
{
    return EraseIC(make_pair(std::string("purpose"), std::make_pair(assetID, strAddress)));
}

bool WalletBatch::HasMasterKey()
{
    return m_batch.Exists(std::make_pair(std::string{ "mkey" }, 1));
}

bool WalletBatch::WriteMasterKey(unsigned int nID, const CMasterKey& kMasterKey)
{
    return WriteIC(std::make_pair(std::string("mkey"), nID), kMasterKey, true);
}

bool WalletBatch::WritePool(uint32_t nPurpose, AssetID assetID, const AccountSetKeyPool& pools)
{
    return WriteIC(
        std::make_pair(std::string("scopedpool"), std::make_pair(nPurpose, assetID)), pools);
}

bool WalletBatch::ErasePool(uint32_t nPurpose, AssetID assetID)
{
    return EraseIC(std::make_pair(std::string("scopedpool"), std::make_pair(nPurpose, assetID)));
}

bool WalletBatch::WriteCScript(AssetID assetID, const uint160& hash, const CScript& redeemScript)
{
    return WriteIC(
        std::make_pair(std::string("cscript"), std::make_pair(assetID, hash)), redeemScript, false);
}

bool WalletBatch::WriteDefaultKey(const CPubKey& vchPubKey)
{
    return WriteIC(std::string("defaultkey"), vchPubKey);
}

bool WalletBatch::WriteMinVersion(int nVersion)
{
    return WriteIC(std::string("minversion"), nVersion);
}

class CWalletScanState {
public:
    unsigned int nKeys;
    unsigned int nCKeys;
    unsigned int nWatchKeys;
    unsigned int nKeyMeta;
    bool fIsEncrypted;
    bool fAnyUnordered;
    int nFileVersion;
    std::vector<uint256> vWalletUpgrade;

    CWalletScanState()
    {
        nKeys = nCKeys = nWatchKeys = nKeyMeta = 0;
        fIsEncrypted = false;
        fAnyUnordered = false;
        nFileVersion = 0;
    }
};

bool ReadKeyValue(CWallet* pwallet, CDataStream& ssKey, CDataStream& ssValue, CWalletScanState& wss,
    std::string& strType, std::string& strErr)
{
    try {
        // Unserialize
        // Taking advantage of the fact that pair serialization
        // is just the two items serialized one after the other
        ssKey >> strType;
        if (strType == "name") {
            std::pair<AssetID, std::string> addressBookEntry;
            ssKey >> addressBookEntry;
            ssValue
                >> pwallet->mapAddressBook[addressBookEntry.first][addressBookEntry.second].name;
        } else if (strType == "purpose") {
            std::pair<AssetID, std::string> addressBookEntry;
            ssKey >> addressBookEntry;
            ssValue
                >> pwallet->mapAddressBook[addressBookEntry.first][addressBookEntry.second].purpose;
        } else if (strType == "mkey") {
            unsigned int nID;
            ssKey >> nID;
            CMasterKey kMasterKey;
            ssValue >> kMasterKey;
            if (pwallet->mapMasterKeys.count(nID) != 0) {
                strErr
                    = strprintf("Error reading wallet database: duplicate CMasterKey id %u", nID);
                return false;
            }
            pwallet->mapMasterKeys[nID] = kMasterKey;
            if (pwallet->nMasterKeyMaxID < nID)
                pwallet->nMasterKeyMaxID = nID;
        } else if (strType == "keymeta") {
            CKeyID keyID;
            std::pair<AssetID, CPubKey> entry;
            ssKey >> entry;
            CPubKey vchPubKey = entry.second;
            keyID = vchPubKey.GetID();

            CKeyMetadata keyMeta;
            ssValue >> keyMeta;
            wss.nKeyMeta++;

            pwallet->LoadKeyMetadata(entry.first, keyID, keyMeta);
        } else if (strType == "defaultkey") {
            ssValue >> pwallet->vchDefaultKey;
        } else if (strType == "version") {
            ssValue >> wss.nFileVersion;
            if (wss.nFileVersion == 10300)
                wss.nFileVersion = 300;
        } /*else if (strType == "cscript") {
            std::pair<AssetID, uint160> entry;
            ssKey >> entry;
            uint160 hash = entry.second;
            CScript script;
            ssValue >> *(CScriptBase*)(&script);
            if (!pwallet->LoadCScript(entry.first, script)) {
                strErr = "Error reading wallet database: LoadCScript failed";
                return false;
            }
        } else if (strType == "pool") {
            AssetID assetID;
            ssKey >> assetID;
            ssValue >> pwallet->_keyPools[assetID];
        }*/
        else if (strType == "scopedpool") {
            std::pair<uint32_t, AssetID> key;
            ssKey >> key;
            ssValue >> pwallet->_keyPools[key.first][key.second];
        } else if (strType == "orderposnext") {
            ssValue >> pwallet->nOrderPosNext;
        } else if (strType == "destdata") {
            std::string strAddress, strKey, strValue;
            ssKey >> strAddress;
            ssKey >> strKey;
            ssValue >> strValue;
        } else if (strType == keys::SCOPED_HD_CHAIN) {
            CHDChain chain;
            ssValue >> chain;
            if (!pwallet->SetHDChain(nullptr, chain, true)) {
                strErr = "Error reading wallet database: SetHDChain failed";
                return false;
            }
        } else if (strType == keys::CRYPTED_SCOPED_HD_CHAIN) {
            CHDChain chain;
            ssValue >> chain;
            if (!pwallet->SetCryptedHDChain(nullptr, chain, true)) {
                strErr = "Error reading wallet database: SetHDCryptedChain failed";
                return false;
            }
        } else if (strType == "hdpubkey") {
            std::pair<AssetID, CPubKey> entry;
            ssKey >> entry;
            CPubKey vchPubKey = entry.second;

            CHDPubKey hdPubKey;
            ssValue >> hdPubKey;

            if (vchPubKey != hdPubKey.extPubKey.pubkey) {
                strErr = "Error reading wallet database: CHDPubKey corrupt";
                return false;
            }
            if (!pwallet->LoadHDPubKey(entry.first, hdPubKey)) {
                strErr = "Error reading wallet database: LoadHDPubKey failed";
                return false;
            }
        } else if (strType == "addresstype") {
            ssValue >> pwallet->addressTypes;
        } else if (strType == keys::legacy::MAIN_HDCHAIN || strType == keys::legacy::AUX_HDCHAIN) {
            strErr = "Unsupported version: HD chain not supported";
            return false;
        }
    } catch (...) {
        return false;
    }
    return true;
}

static bool IsKeyType(std::string strType)
{
    return (strType == "key" || strType == "wkey" || strType == "mkey" || strType == "ckey"
        || strType == keys::legacy::AUX_HDCHAIN || strType == keys::legacy::MAIN_HDCHAIN
        || strType == keys::SCOPED_HD_CHAIN || strType == keys::CRYPTED_SCOPED_HD_CHAIN);
}

DBErrors WalletBatch::LoadWallet(CWallet* pwallet)
{
    pwallet->vchDefaultKey = CPubKey();
    CWalletScanState wss;
    bool fNoncriticalErrors = false;
    DBErrors result = DB_LOAD_OK;

    LOCK(pwallet->cs_wallet);
    try {
        int nMinVersion = 0;
        if (m_batch.Read(std::string("minversion"), nMinVersion)) {
            if (nMinVersion > FEATURE_LATEST)
                return DB_TOO_NEW;
            pwallet->LoadMinVersion(nMinVersion);
        }

        // Get cursor
        Dbc* pcursor = m_batch.GetCursor();
        if (!pcursor) {
            LogPrintf("Error getting wallet database cursor\n");
            return DB_CORRUPT;
        }

        while (true) {
            // Read next record
            CDataStream ssKey(SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(SER_DISK, CLIENT_VERSION);
            int ret = m_batch.ReadAtCursor(pcursor, ssKey, ssValue);
            if (ret == DB_NOTFOUND)
                break;
            else if (ret != 0) {
                LogPrintf("Error reading next record from wallet database\n");
                return DB_CORRUPT;
            }

            // Try to be tolerant of single corrupt records:
            std::string strType, strErr;
            if (!ReadKeyValue(pwallet, ssKey, ssValue, wss, strType, strErr)) {
                // losing keys is considered a catastrophic error, anything else
                // we assume the user can live with:
                if (IsKeyType(strType)) {
                    result = DB_CORRUPT;
                }
            }
            if (!strErr.empty())
                LogPrintf("%s\n", strErr);
        }
        pcursor->close();

    } catch (const boost::thread_interrupted&) {
        throw;
    } catch (...) {
        result = DB_CORRUPT;
    }

    if (fNoncriticalErrors && result == DB_LOAD_OK)
        result = DB_NONCRITICAL_ERROR;

    // Any wallet corruption at all: skip any rewriting or
    // upgrading, we don't want to make it worse.
    if (result != DB_LOAD_OK)
        return result;

    LogPrintf("nFileVersion = %d\n", wss.nFileVersion);

    LogPrintf("Keys: %u plaintext, %u encrypted, %u w/ metadata, %u total\n", wss.nKeys, wss.nCKeys,
        wss.nKeyMeta, wss.nKeys + wss.nCKeys);

    // nTimeFirstKey is only reliable if all keys have metadata
    if ((wss.nKeys + wss.nCKeys + wss.nWatchKeys) != wss.nKeyMeta)
        pwallet->UpdateTimeFirstKey(1);

    // Rewrite encrypted wallets of versions 0.4.0 and 0.5.0rc:
    if (wss.fIsEncrypted && (wss.nFileVersion == 40000 || wss.nFileVersion == 50000))
        return DB_NEED_REWRITE;

    if (wss.nFileVersion < CLIENT_VERSION) // Update
        m_batch.Write(keys::VERSION, CLIENT_VERSION);

    return result;
}

#if 0
//
// Try to (very carefully!) recover wallet file if there is a problem.
//
bool WalletBatch::Recover(CDBEnv& dbenv, const std::string& filename, bool fOnlyKeys)
{
    // Recovery procedure:
    // move wallet file to wallet.timestamp.bak
    // Call Salvage with fAggressive=true to
    // get as much data as possible.
    // Rewrite salvaged data to fresh wallet file
    // Set -rescan so any missing transactions will be
    // found.
    int64_t now = GetTime();
    std::string newFilename = strprintf("wallet.%d.bak", now);

    int result = dbenv.dbenv->dbrename(NULL, filename.c_str(), NULL,
                                       newFilename.c_str(), DB_AUTO_COMMIT);
    if (result == 0)
        LogPrintf("Renamed %s to %s\n", filename, newFilename);
    else
    {
        LogPrintf("Failed to rename %s to %s\n", filename, newFilename);
        return false;
    }

    std::vector<CDBEnv::KeyValPair> salvagedData;
    bool fSuccess = dbenv.Salvage(newFilename, true, salvagedData);
    if (salvagedData.empty())
    {
        LogPrintf("Salvage(aggressive) found no records in %s.\n", newFilename);
        return false;
    }
    LogPrintf("Salvage(aggressive) found %u records\n", salvagedData.size());

    std::unique_ptr<Db> pdbCopy(new Db(dbenv.dbenv, 0));
    int ret = pdbCopy->open(NULL,               // Txn pointer
                            filename.c_str(),   // Filename
                            "main",             // Logical db name
                            DB_BTREE,           // Database type
                            DB_CREATE,          // Flags
                            0);
    if (ret > 0)
    {
        LogPrintf("Cannot create database file %s\n", filename);
        return false;
    }
    CWallet dummyWallet;
    CWalletScanState wss;

    DbTxn* ptxn = dbenv.TxnBegin();
    BOOST_FOREACH(CDBEnv::KeyValPair& row, salvagedData)
    {
        if (fOnlyKeys)
        {
            CDataStream ssKey(row.first, SER_DISK, CLIENT_VERSION);
            CDataStream ssValue(row.second, SER_DISK, CLIENT_VERSION);
            std::string strType, strErr;
            bool fReadOK;
            {
                // Required in LoadKeyMetadata():
                LOCK(dummyWallet.cs_wallet);
                fReadOK = ReadKeyValue(&dummyWallet, ssKey, ssValue,
                                       wss, strType, strErr);
            }
            if (!IsKeyType(strType) && strType != "hdpubkey")
                continue;
            if (!fReadOK)
            {
                LogPrintf("WARNING: WalletBatch::Recover skipping %s: %s\n", strType, strErr);
                continue;
            }
        }
        Dbt datKey(&row.first[0], row.first.size());
        Dbt datValue(&row.second[0], row.second.size());
        int ret2 = pdbCopy->put(ptxn, &datKey, &datValue, DB_NOOVERWRITE);
        if (ret2 > 0)
            fSuccess = false;
    }
    ptxn->commit(0);
    pdbCopy->close(0);

    return fSuccess;
}

bool WalletBatch::Recover(CDBEnv& dbenv, const std::string& filename)
{
    return WalletBatch::Recover(dbenv, filename, false);
}

#endif

bool WalletBatch::WriteDestData(
    const std::string& address, const std::string& key, const std::string& value)
{
    return WriteIC(std::make_pair(std::string("destdata"), std::make_pair(address, key)), value);
}

bool WalletBatch::EraseDestData(const std::string& address, const std::string& key)
{
    return EraseIC(std::make_pair(std::string("destdata"), std::make_pair(address, key)));
}

bool WalletBatch::WriteAddressType(const std::map<AssetID, std::string> addressType)
{
    return WriteIC(std::string("addresstype"), addressType);
}

bool WalletBatch::WriteHDChain(const CHDChain& chain)
{
    return WriteIC(keys::SCOPED_HD_CHAIN, chain);
}

bool WalletBatch::WriteCryptedHDChain(const CHDChain& chain)
{
    if (!WriteIC(keys::CRYPTED_SCOPED_HD_CHAIN, chain))
        return false;

    EraseIC(keys::SCOPED_HD_CHAIN);
    return true;
}

bool WalletBatch::WriteHDPubKey(
    AssetID assetID, const CHDPubKey& hdPubKey, const CKeyMetadata& keyMeta)
{
    if (!WriteIC(std::make_pair(
                     std::string("keymeta"), std::make_pair(assetID, hdPubKey.extPubKey.pubkey)),
            keyMeta, false))
        return false;

    return WriteIC(
        std::make_pair(std::string("hdpubkey"), std::make_pair(assetID, hdPubKey.extPubKey.pubkey)),
        hdPubKey, false);
}

bool WalletBatch::VerifyEnvironment(const fs::path& wallet_path, std::string& errorStr)
{
    return BerkeleyBatch::VerifyEnvironment(wallet_path, errorStr);
}

bool WalletBatch::VerifyDatabaseFile(
    const fs::path& wallet_path, std::string& warningStr, std::string& errorStr)
{
    // TODO: fix this
    return BerkeleyBatch::VerifyDatabaseFile(wallet_path, warningStr, errorStr, nullptr);
    //    return BerkeleyBatch::VerifyDatabaseFile(wallet_path, warningStr, errorStr,
    //    WalletBatch::Recover);
}

bool WalletBatch::TxnBegin()
{
    return m_batch.TxnBegin();
}

bool WalletBatch::TxnCommit()
{
    return m_batch.TxnCommit();
}

bool WalletBatch::TxnAbort()
{
    return m_batch.TxnAbort();
}
}
