// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OUTPUTTYPE_H
#define BITCOIN_OUTPUTTYPE_H

#include <keystore.h>
#include <script/standard.h>

#include <string>
#include <vector>

namespace bitcoin {

class CPubKey;

enum class OutputType {
    LEGACY,
    P2SH_SEGWIT,
    BECH32
};

/**
 * Get a destination of the requested type (if possible) to the specified key.
 * The caller must make sure LearnRelatedScripts has been called beforehand.
 */
CTxDestination GetDestinationForKey(const CKeyID& keyID, OutputType);

/** Get all destinations (potentially) supported by the wallet for the given key. */
std::vector<CTxDestination> GetAllDestinationsForKey(const CPubKey& key);
}

#endif // BITCOIN_OUTPUTTYPE_H
