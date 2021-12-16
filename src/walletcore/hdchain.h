// Copyright (c) 2014-2017 The Dash Core developers
// Distributed under the MIT software license, see the accompanying
#ifndef DASH_HDCHAIN_H
#define DASH_HDCHAIN_H

#include "chainparams.hpp"
#include "key.h"
#include "sync.h"

#include <optional>

namespace bitcoin {

static const uint32_t HD_PURPOSE_WALLET_KEYS = 44;
static const uint32_t HD_PURPUSE_LND = 1017;
static const uint32_t HD_PURPUSE_UTILITY = 384;

/* hd account data model */
class CHDAccount {
public:
    uint32_t nExternalChainCounter;
    uint32_t nInternalChainCounter;

    CHDAccount()
        : nExternalChainCounter(0)
        , nInternalChainCounter(0)
    {
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(nExternalChainCounter);
        READWRITE(nInternalChainCounter);
    }
};

// Represents scope cointype'/account'/branch'/index
// in general scheme
class CHDChainScope {
private:
    uint32_t nVersion{ CURRENT_VERSION };
    using AccountsMap = std::map<uint32_t, CHDAccount>;
    std::map<uint32_t, AccountsMap> accountsByCoin;

    // critical section to protect accounts
    mutable CCriticalSection cs_accounts;

    static const int CURRENT_VERSION = 1;

public:
    std::optional<uint32_t> nPurpose; // memonly

    CHDChainScope() { SetNull(); }
    CHDChainScope(const CHDChainScope& other)
        : accountsByCoin(other.accountsByCoin)
    {
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        LOCK(cs_accounts);
        READWRITE(nVersion);
        READWRITE(accountsByCoin);
    }

    void SetNull();

    void swap(CHDChainScope& first, CHDChainScope& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.accountsByCoin, second.accountsByCoin);
    }

    CHDChainScope& operator=(CHDChainScope from)
    {
        swap(*this, from);
        return *this;
    }

    void AddAccount(uint32_t coinType);
    bool GetAccount(uint32_t coinType, uint32_t nAccountIndex, CHDAccount& hdAccountRet) const;
    bool SetAccount(uint32_t coinType, uint32_t nAccountIndex, const CHDAccount& hdAccount);
    size_t CountAccounts(uint32_t coinType);
};

class CLegacyHDCHain {
public:
    static const int CURRENT_VERSION = 1;
    int nVersion;
    uint256 id;
    bool fCrypted;

    SecureVector vchSeed;
    SecureVector vchMnemonic;
    SecureVector vchMnemonicPassphrase;

    using AccountsMap = std::map<uint32_t, CHDAccount>;
    std::map<uint32_t, AccountsMap> accountsByCoin;
    uint32_t nPurpose;

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->nVersion);
        READWRITE(id);
        READWRITE(nPurpose);
        READWRITE(fCrypted);
        READWRITE(vchSeed);
        READWRITE(vchMnemonic);
        READWRITE(vchMnemonicPassphrase);
        READWRITE(accountsByCoin);
    }
};

/* simple HD chain data model */
class CHDChain {
private:
    static const int CURRENT_VERSION = 1;
    int nVersion;
    uint256 id;
    bool fCrypted;

    SecureVector vchSeed;
    SecureVector vchMnemonic;
    SecureVector vchMnemonicPassphrase;

    // uint32_t is purpose: in derivation scheme:
    // m/purpose'/cointype'/account'/branch'/index
    std::map<uint32_t, CHDChainScope> mapChainScopes;

    // critical section to protect accounts
    mutable CCriticalSection cs_scopes;

public:
    CHDChain() { SetNull(); }
    CHDChain(const CLegacyHDCHain& other)
        : nVersion(other.nVersion)
        , id(other.id)
        , fCrypted(other.fCrypted)
        , vchSeed(other.vchSeed)
        , vchMnemonic(other.vchMnemonic)
        , vchMnemonicPassphrase(other.vchMnemonicPassphrase)
    {
    }
    CHDChain(const CHDChain& other)
        : nVersion(other.nVersion)
        , id(other.id)
        , fCrypted(other.fCrypted)
        , vchSeed(other.vchSeed)
        , vchMnemonic(other.vchMnemonic)
        , vchMnemonicPassphrase(other.vchMnemonicPassphrase)
        , mapChainScopes(other.mapChainScopes)
    {
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        LOCK(cs_scopes);
        READWRITE(this->nVersion);
        READWRITE(id);
        READWRITE(fCrypted);
        READWRITE(vchSeed);
        READWRITE(vchMnemonic);
        READWRITE(vchMnemonicPassphrase);
        READWRITE(mapChainScopes);
    }

    void swap(CHDChain& first, CHDChain& second) // nothrow
    {
        // enable ADL (not necessary in our case, but good practice)
        using std::swap;

        // by swapping the members of two classes,
        // the two classes are effectively swapped
        swap(first.nVersion, second.nVersion);
        swap(first.id, second.id);
        swap(first.fCrypted, second.fCrypted);
        swap(first.vchSeed, second.vchSeed);
        swap(first.vchMnemonic, second.vchMnemonic);
        swap(first.vchMnemonicPassphrase, second.vchMnemonicPassphrase);
        swap(first.mapChainScopes, second.mapChainScopes);
    }
    CHDChain& operator=(CHDChain from)
    {
        swap(*this, from);
        return *this;
    }

    bool SetNull();
    bool IsNull() const;

    void SetCrypted(bool fCryptedIn);
    bool IsCrypted() const;

    void Debug(const std::string& strName) const;

    bool SetMnemonic(
        const SecureVector& vchMnemonic, const SecureVector& vchMnemonicPassphrase, bool fUpdateID);
    bool SetMnemonic(
        const SecureString& ssMnemonic, const SecureString& ssMnemonicPassphrase, bool fUpdateID);
    bool GetMnemonic(SecureVector& vchMnemonicRet, SecureVector& vchMnemonicPassphraseRet) const;
    bool GetMnemonic(SecureString& ssMnemonicRet, SecureString& ssMnemonicPassphraseRet) const;

    bool SetSeed(const SecureVector& vchSeedIn, bool fUpdateID);
    SecureVector GetSeed() const;

    uint256 GetID() const { return id; }

    uint256 GetSeedHash();

    CHDChainScope GetChainScope(uint32_t nPurpose);
    void SetChainScope(uint32_t nPurpose, const CHDChainScope& scope);

    void DeriveChildExtKey(uint32_t nPurpose, uint32_t nCoinType, uint32_t nAccountIndex,
        bool fInternal, uint32_t nChildIndex, CExtKey& extKeyRet) const;
};

/* hd pubkey data model */
class CHDPubKey {
private:
    static const int CURRENT_VERSION = 2;
    int nVersion;

public:
    CExtPubKey extPubKey;
    uint256 hdchainID;
    uint32_t nPurpose;
    uint32_t nCoinType;
    uint32_t nAccountIndex;
    uint32_t nChangeIndex;

    CHDPubKey()
        : nVersion(CHDPubKey::CURRENT_VERSION)
        , nPurpose(0)
        , nCoinType(0)
        , nAccountIndex(0)
        , nChangeIndex(0)
    {
    }

    ADD_SERIALIZE_METHODS;
    template <typename Stream, typename Operation>
    inline void SerializationOp(Stream& s, Operation ser_action)
    {
        READWRITE(this->nVersion);
        READWRITE(extPubKey);
        READWRITE(hdchainID);
        READWRITE(nCoinType);
        READWRITE(nAccountIndex);
        READWRITE(nChangeIndex);
        if (nVersion > 1) {
            READWRITE(nPurpose);
        }
    }

    std::string GetKeyPath() const;
};
} // namespace bitcoin

#endif // DASH_HDCHAIN_H
