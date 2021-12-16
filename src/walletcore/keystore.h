// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

#include "hdchain.h"
#include "key.h"
#include "pubkey.h"
#include "sync.h"

#include <boost/signals2/signal.hpp>
#include <boost/variant.hpp>

namespace bitcoin {

class CScript;
class CScriptID;

/** A virtual base class for key stores */
class CKeyStore {
protected:
    mutable CCriticalSection cs_KeyStore;

public:
    virtual ~CKeyStore() {}

    //! Check whether a key corresponding to a given address is present in the store.
    virtual bool HaveKey(uint32_t nCoinType, const CKeyID& address) const = 0;
    virtual bool GetKey(uint32_t nCoinType, const CKeyID& address, CKey& keyOut) const = 0;
    virtual bool GetPubKey(uint32_t nCoinType, const CKeyID& address, CPubKey& vchPubKeyOut) const = 0;

    //! Support for BIP 0013 : see https://github.com/bitcoin/bips/blob/master/bip-0013.mediawiki
//    virtual bool AddCScript(uint32_t nCoinType, const CScript& redeemScript) = 0;
//    virtual bool HaveCScript(uint32_t nCoinType, const CScriptID& hash) const = 0;
//    virtual bool GetCScript(uint32_t nCoinType, const CScriptID& hash, CScript& redeemScriptOut) const = 0;

    virtual bool GetHDChain(CHDChain& hdChainRet) const = 0;
};

class CHDKeyStore : public CKeyStore {
protected:
    CHDChain hdChainInternal;

    using HDPubKeys = std::map<CKeyID, CHDPubKey>;
    std::map<uint32_t, HDPubKeys> mapHdPubKeys; //<! memory map of HD extended pubkeys

protected:
    virtual bool SetHDChain(const CHDChain& chain);

public:
    virtual bool GetHDChain(CHDChain& hdChainRet) const;
    //! loads a HDPubKey into the wallets memory
    bool LoadHDPubKey(uint32_t nCoinType, const CHDPubKey& hdPubKey);
    bool GetHDPubKey(uint32_t nCoinType, const CKeyID &keyID, CHDPubKey &hdPubKey);
};

typedef std::map<CKeyID, CKey> KeyMap;
using AssetKeyMap = std::map<uint32_t, KeyMap>;
typedef std::map<CScriptID, CScript> ScriptMap;
using AssetScriptMap = std::map<uint32_t, ScriptMap>;
typedef std::set<CScript> MultiSigScriptSet;
using AssetMultiSigScriptSet = std::map<uint32_t, MultiSigScriptSet>;

typedef std::vector<unsigned char, secure_allocator<unsigned char>> CKeyingMaterial;
typedef std::map<CKeyID, std::pair<CPubKey, std::vector<unsigned char>>> CryptedKeyMap;
using AssetCryptedKeyMap = std::map<uint32_t, CryptedKeyMap>;
}

#endif // BITCOIN_KEYSTORE_H
