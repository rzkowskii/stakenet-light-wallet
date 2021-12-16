// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <outputtype.h>

#include <keystore.h>
#include <pubkey.h>
#include <script/script.h>
#include <script/standard.h>

#include <assert.h>
#include <string>

namespace bitcoin {

static const std::string OUTPUT_TYPE_STRING_LEGACY = "legacy";
static const std::string OUTPUT_TYPE_STRING_P2SH_SEGWIT = "p2sh-segwit";
static const std::string OUTPUT_TYPE_STRING_BECH32 = "bech32";

CTxDestination GetDestinationForKey(const CKeyID& keyID, OutputType type)
{
    switch (type) {
    case OutputType::LEGACY:
        return keyID;
    case OutputType::P2SH_SEGWIT:
    case OutputType::BECH32: {
        CTxDestination witdest = WitnessV0KeyHash(keyID);
        CScript witprog = GetScriptForDestination(witdest);
        if (type == OutputType::P2SH_SEGWIT) {
            return CScriptID(witprog);
        } else {
            return witdest;
        }
    }
    default:
        assert(false);
    }
}

std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey& key)
{
    CKeyID keyid = key.GetID();
    if (key.IsCompressed()) {
        CTxDestination segwit = WitnessV0KeyHash(keyid);
        CTxDestination p2sh = CScriptID(GetScriptForDestination(segwit));
        return std::vector<CTxDestination>{ std::move(keyid), std::move(p2sh), std::move(segwit) };
    } else {
        return std::vector<CTxDestination>{ std::move(keyid) };
    }
}
}
