// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "keystore.h"

#include "crypter.h"
#include "key.h"
#include "script/script.h"
#include "script/standard.h"

#include <boost/foreach.hpp>

namespace bitcoin {

bool CHDKeyStore::SetHDChain(const CHDChain &chain)
{
    LOCK(cs_KeyStore);
    hdChainInternal = chain;
    return true;
}

bool CHDKeyStore::GetHDChain(CHDChain &hdChainRet) const
{
    LOCK(cs_KeyStore);
    hdChainRet = hdChainInternal;
    return !hdChainInternal.IsNull();
}

bool CHDKeyStore::LoadHDPubKey(uint32_t nCoinType, const CHDPubKey &hdPubKey)
{
    LOCK(cs_KeyStore);
    mapHdPubKeys[nCoinType][hdPubKey.extPubKey.pubkey.GetID()] = hdPubKey;
    return true;
}

bool CHDKeyStore::GetHDPubKey(uint32_t nCoinType, const CKeyID &keyID, CHDPubKey &hdPubKey)
{
    if (mapHdPubKeys.count(nCoinType) > 0) {
        if (mapHdPubKeys.at(nCoinType).count(keyID) > 0) {
            hdPubKey = mapHdPubKeys.at(nCoinType).at(keyID);
            return true;
        }
    }

    return false;
}

}
