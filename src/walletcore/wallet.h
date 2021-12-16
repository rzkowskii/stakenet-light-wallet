// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_WALLET_H
#define BITCOIN_WALLET_H

#include "crypter.h"

#include <base58.h>
#include <boost/optional.hpp>
#include <coinselection.h>
#include <hdchain.h>
#include <interfaces.hpp>
#include <key.h>
#include <policy/feerate.h>
#include <script/script.h>
#include <script/standard.h>
#include <walletdb.h>

#include <algorithm>
#include <map>
#include <set>
#include <stdexcept>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include <optional>

namespace bitcoin {

//! if set, all keys will be derived by using BIP39/BIP44
static const bool DEFAULT_USE_HD_WALLET = true;

class CAccountingEntry;
class CCoinControl;
class CScript;
class CHDChain;
class CWalletDB;
class SigningProvider;
struct CoinSelectionParams;

/** (client) version numbers for particular wallet features */
enum WalletFeature {
    FEATURE_BASE = 10500, // the earliest version new wallets supports (only useful for getinfo's
                          // clientversion output)

    FEATURE_WALLETCRYPT = 40000, // wallet encryption
    FEATURE_COMPRPUBKEY = 60000, // compressed public keys
    FEATURE_HD = 120200,

    FEATURE_LATEST = 61000
};

struct CRecipient {
    CScript scriptPubKey;
    CAmount nAmount;
    bool fSubtractFeeFromAmount;
};

/** Address book data */
class CAddressBookData {
public:
    std::string name;
    std::string purpose;

    CAddressBookData() { purpose = "unknown"; }

    typedef std::map<std::string, std::string> StringMap;
    StringMap destdata;
};

/**
 * A CWallet is an extension of a keystore, which also maintains a set of transactions and balances,
 * and provides the ability to create new transactions.
 */
class CWallet : public CCryptoKeyStore {
private:
    WalletBatch* encrypted_batch GUARDED_BY(cs_wallet) = nullptr;
    /** Internal database handle. */
    std::unique_ptr<WalletDatabase> database;

    //! the current wallet version: clients below this version are not able to load the wallet
    int nWalletVersion;

    //! the maximum wallet format version: memory-only variable that specifies to what version this
    //! wallet may be upgraded
    int nWalletMaxVersion;

    int64_t nNextResend;
    int64_t nLastResend;

    void DeriveNewChildKey(WalletBatch& batch, const CHDChain& hdchain,
        const CKeyMetadata& metadata, CKey& secretRet, CHDChainScope& scope, uint32_t nCoinType,
        uint32_t nAccountIndex, bool fInternal /*= false*/);
    void DeriveNewChildKeyHelper(const CKeyMetadata& metadata, CExtKey& childKeyRet,
        const CHDChain& hdChain, CHDChainScope& chainScope, uint32_t nCoinType,
        uint32_t nAccountIndex, bool fInternal);

    void TopUpWalletKeyPool(AssetID assetID);
    void TopUpAuxKeyPool(AssetID assetID, std::vector<uint32_t> accounts, size_t kpSize = 0);

    void DeriveIdentityPubKey();
    void SaveCryptedHDChain(WalletBatch& batch, CHDChain hdChainCurrent);

public:
    /*
     * Main wallet lock.
     * This lock protects all the fields added by CWallet
     *   except for:
     *      strWalletFile (immutable after instantiation)
     */
    mutable CCriticalSection cs_wallet;

    using MetadataKeyMap = std::map<CKeyID, CKeyMetadata>;
    std::map<uint32_t, MetadataKeyMap> mapKeyMetadata;

    using MasterKeyMap = std::map<unsigned int, CMasterKey>;
    MasterKeyMap mapMasterKeys;
    unsigned int nMasterKeyMaxID;

    CWallet() { SetNull(); }

    explicit CWallet(std::string strWalletDirectory);

    ~CWallet() override { delete encrypted_batch; }

    void SetNull()
    {
        nWalletVersion = FEATURE_BASE;
        nWalletMaxVersion = FEATURE_BASE;
        nMasterKeyMaxID = 0;
        encrypted_batch = nullptr;
        nOrderPosNext = 0;
        nNextResend = 0;
        nLastResend = 0;
        nTimeFirstKey = 0;
    }

    int64_t nOrderPosNext;
    std::map<uint256, int> mapRequestCount;

    std::map<uint32_t, AssetsSetKeyPools> _keyPools;

    // Address - key
    using MapAddressBook = std::map<std::string, CAddressBookData>;
    std::map<AssetID, MapAddressBook> mapAddressBook;
    using AddressesList = std::vector<std::string>;

    CPubKey vchDefaultKey;

    int64_t nTimeFirstKey;

    std::map<AssetID, std::string> addressTypes;

    std::string identityPubKey; //<! memonly identity pubkey derived from seed

    /** Get database handle used by this wallet. Ideally this function would
     * not be necessary.
     */
    WalletDatabase& GetDBHandle() { return *database; }

    //! check whether we are allowed to upgrade (or already support) to the named feature
    bool CanSupportFeature(enum WalletFeature wf)
    {
        AssertLockHeld(cs_wallet);
        return nWalletMaxVersion >= wf;
    }

    std::vector<CKeyID> GetKeyPoolKeys(AssetID assetID, uint32_t nAccountIndex, bool fInternal);
    CPubKey GetLastUnusedKey(AssetID nCoinType, uint32_t nAccountIndex, bool fInternal);
    void MarkAddressAsUsed(CTxDestination address, CChainParams chainParams, AssetID assetID);

    void RecoverAuxChain(
        std::vector<uint32_t> assets, std::vector<uint32_t> accounts, uint32_t recoveryWindow);

    AddressesList GetAddressesByAssetID(AssetID assetID, bool fInternal) const;

    bool SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue,
        std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet,
        CoinSelectionParams& coin_selection_params, bool& bnb_used) const;

    bool SelectCoinsMinConf(const CAmount& nTargetValue,
        const CoinEligibilityFilter& eligibility_filter, std::vector<OutputGroup> groups,
        std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet,
        const CoinSelectionParams& coin_selection_params, bool& bnb_used) const;

    std::vector<OutputGroup> GroupOutputs(
        const std::vector<COutput>& outputs, bool single_coin) const;

    bool CreateTransaction(interfaces::Chain::Lock& locked_chain,
        const interfaces::CoinsView& coinsView, interfaces::ReserveKeySource& reserveKeySource,
        uint32_t nCoinType, const std::vector<CRecipient>& vecSend, CTransactionRef& tx,
        boost::optional<CFeeRate> feeRate, CAmount& nFeeRet, int& nChangePosInOut,
        std::string& strFailReason);

    //  keystore implementation
    // Generate a new key
    CPubKey GenerateNewKey(WalletBatch& batch, uint32_t nPurpose, uint32_t nCoinType,
        uint32_t nAccountIndex, bool fInternal);
    CPubKey GetNewAuxKey(AssetID nCoinType, uint32_t nAccountIndex);

    bool HaveKey(uint32_t nCoinType, const CKeyID& address) const override;
    //! GetPubKey implementation that also checks the mapHdPubKeys
    bool GetPubKey(uint32_t nCoinType, const CKeyID& address, CPubKey& vchPubKeyOut) const override;
    //! GetKey implementation that can derive a HD private key on the fly
    bool GetKey(uint32_t nCoinType, const CKeyID& address, CKey& keyOut) const override;
    //! Adds a HDPubKey into the wallet(database)
    bool AddHDPubKey(WalletBatch& batch, const CHDChain& hdChain, uint32_t nPurpose,
        uint32_t nCoinType, uint32_t nAccountIndex, const CExtPubKey& extPubKey, bool fInternal);
    //! Load metadata (used by LoadWallet)
    bool LoadKeyMetadata(uint32_t nCoinType, const CKeyID& keyID, const CKeyMetadata& metadata);

    bool FindHDPubKey(uint32_t nCoinType, uint32_t nAccountIndex, bool internal, uint32_t nIndex,
        CPubKey &pubkey);

    bool LoadMinVersion(int nVersion);
    void UpdateTimeFirstKey(int64_t nCreateTime);

    void TopUpKeyPool(size_t kpSize = 0);
    void TopUpKeyPoolByAsset(uint32_t nPurpose, AssetID assetID, std::vector<uint32_t> accounts,
        std::optional<int> internalSize = {}, std::optional<int> externalSize = {});

    //    bool AddCScript(uint32_t nCoinType, const CScript& redeemScript) override;
    //    bool LoadCScript(uint32_t nCoinType, const CScript& redeemScript);

    bool Unlock(const SecureString& strWalletPassphrase);
    bool ChangeWalletPassphrase(
        const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase);
    bool EncryptWallet(const SecureString& strWalletPassphrase);

    bool IsEncrypted();

    void GetKeyBirthTimes(std::map<CKeyID, int64_t>& mapKeyBirth) const;

    std::unique_ptr<SigningProvider> CreateSignatureProvider(uint32_t nCoinType) const;

    /**
     * HD Wallet Functions
     */

    /* Generates a new HD chain */
    void GenerateNewHDChain(std::string strSeed = std::string(),
        std::string mnemonic = std::string(), std::string mnemonicpassphrase = std::string());
    /* Set the HD chain model (chain child index counters) */
    bool SetHDChain(WalletBatch* batch, const CHDChain& chain, bool memonly);
    bool SetCryptedHDChain(WalletBatch* batch, const CHDChain& chain, bool memonly);
    bool GetDecryptedHDChain(CHDChain& hdChainRet) const;

    DBErrors LoadWallet(bool& fFirstRunRet);
    void InitHDChainAccounts(std::vector<AssetID> assets);

    bool SetAddressBook(WalletBatch& batch, const std::string& address, AssetID assetID,
        const std::string& strName = std::string(), const std::string& purpose = std::string());

    bool DelAddressBook(const std::string& address);

    void Inventory(const uint256& hash)
    {
        {
            LOCK(cs_wallet);
            std::map<uint256, int>::iterator mi = mapRequestCount.find(hash);
            if (mi != mapRequestCount.end())
                (*mi).second++;
        }
    }

    unsigned int GetKeyPoolSize() const;

    //! signify that a particular wallet feature is now used. this may change nWalletVersion and
    //! nWalletMaxVersion if those are lower
    bool SetMinVersion(enum WalletFeature, WalletBatch* batch_in = NULL, bool fExplicit = false);

    //! change which version we're allowed to upgrade to (note that this does not immediately imply
    //! upgrading to that format)
    bool SetMaxVersion(int nVersion);

    //! get the current wallet format (the oldest client version guaranteed to understand this
    //! wallet)
    int GetVersion()
    {
        LOCK(cs_wallet);
        return nWalletVersion;
    }

    std::string GetAddressType(AssetID assetID) const;
    void SetAddressType(AssetID assetID, std::string addressTypes);
};
}

#endif // BITCOIN_WALLET_H
