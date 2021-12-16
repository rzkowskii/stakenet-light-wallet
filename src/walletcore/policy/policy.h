// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_POLICY_POLICY_H
#define BITCOIN_POLICY_POLICY_H

#include <policy/feerate.h>
#include <script/standard.h>
#include <stdint.h>

#include <string>

namespace bitcoin {

class CCoinsViewCache;
class CTxOut;

/** The maximum allowed size for a serialized block, in bytes (only for buffer size limits) */
static const unsigned int MAX_BLOCK_SERIALIZED_SIZE = 4000000;
/** The maximum allowed weight for a block, see BIP 141 (network rule) */
static const unsigned int MAX_BLOCK_WEIGHT = 4000000;
/** The maximum allowed number of signature check operations in a block (network rule) */
static const int64_t MAX_BLOCK_SIGOPS_COST = 80000;
/** Coinbase transaction outputs can only be spent after this number of new blocks (network rule) */
static const int COINBASE_MATURITY = 20;

static const int WITNESS_SCALE_FACTOR = 4;

static const size_t MIN_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 60; // 60 is the lower bound for the size of a valid serialized CTransaction
static const size_t MIN_SERIALIZABLE_TRANSACTION_WEIGHT = WITNESS_SCALE_FACTOR * 10; // 10 is the lower bound for the size of a serialized CTransaction

/** Flags for nSequence and nLockTime locks */
/** Interpret sequence numbers as relative lock-time constraints. */
static constexpr unsigned int LOCKTIME_VERIFY_SEQUENCE = (1 << 0);
/** Use GetMedianTimePast() instead of nTime for end point timestamp. */
static constexpr unsigned int LOCKTIME_MEDIAN_TIME_PAST = (1 << 1);

/** Default for -blockmaxweight, which controls the range of block weights the mining code will create **/
static const unsigned int DEFAULT_BLOCK_MAX_WEIGHT = MAX_BLOCK_WEIGHT - 4000;
/** Default for -blockmintxfee, which sets the minimum feerate for a transaction in blocks created by mining code **/
static const unsigned int DEFAULT_BLOCK_MIN_TX_FEE = 1000;
/** The maximum weight for transactions we're willing to relay/mine */
static const unsigned int MAX_STANDARD_TX_WEIGHT = 400000;
/** The minimum non-witness size for transactions we're willing to relay/mine (1 segwit input + 1 P2WPKH output = 82 bytes) */
static const unsigned int MIN_STANDARD_TX_NONWITNESS_SIZE = 82;
/** Maximum number of signature check operations in an IsStandard() P2SH script */
static const unsigned int MAX_P2SH_SIGOPS = 15;
/** The maximum number of sigops we're willing to relay/mine in a single tx */
static const unsigned int MAX_STANDARD_TX_SIGOPS_COST = MAX_BLOCK_SIGOPS_COST / 5;
/** Default for -maxmempool, maximum megabytes of mempool memory usage */
static const unsigned int DEFAULT_MAX_MEMPOOL_SIZE = 300;
/** Default for -incrementalrelayfee, which sets the minimum feerate increase for mempool limiting or BIP 125 replacement **/
static const unsigned int DEFAULT_INCREMENTAL_RELAY_FEE = 1000;
/** Default for -bytespersigop */
static const unsigned int DEFAULT_BYTES_PER_SIGOP = 20;
/** The maximum number of witness stack items in a standard P2WSH script */
static const unsigned int MAX_STANDARD_P2WSH_STACK_ITEMS = 100;
/** The maximum size of each witness stack item in a standard P2WSH script */
static const unsigned int MAX_STANDARD_P2WSH_STACK_ITEM_SIZE = 80;
/** The maximum size of a standard witnessScript */
static const unsigned int MAX_STANDARD_P2WSH_SCRIPT_SIZE = 3600;
/** Min feerate for defining dust. Historically this has been based on the
 * minRelayTxFee, however changing the dust limit changes which transactions are
 * standard and should be done with care and ideally rarely. It makes sense to
 * only increase the dust limit after prior releases were already not creating
 * outputs below the new threshold */
static const unsigned int DUST_RELAY_TX_FEE = 3000;

/** Used as the flags parameter to sequence and nLocktime checks in non-consensus code. */
static constexpr unsigned int STANDARD_LOCKTIME_VERIFY_FLAGS = LOCKTIME_VERIFY_SEQUENCE | LOCKTIME_MEDIAN_TIME_PAST;

CAmount GetDustThreshold(const CTxOut& txout, const CFeeRate& dustRelayFee);

bool IsDust(const CTxOut& txout, const CFeeRate& dustRelayFee);

bool IsStandard(const CScript& scriptPubKey, txnouttype& whichType);
/**
     * Check for standard transaction types
     * @return True if all outputs (scriptPubKeys) use only standard transaction forms
     */
bool IsStandardTx(const CTransaction& tx, std::string& reason);
/**
     * Check if the transaction is over standard P2WSH resources limit:
     * 3600bytes witnessScript size, 80bytes per witness stack element, 100 witness stack elements
     * These limits are adequate for multi-signature up to n-of-100 using OP_CHECKSIG, OP_ADD, and OP_EQUAL,
     */
//bool IsWitnessStandard(const CTransaction& tx, const CCoinsViewCache& mapInputs);

extern CFeeRate incrementalRelayFee;
extern CFeeRate dustRelayFee;
extern unsigned int nBytesPerSigOp;

/** Compute the virtual transaction size (weight reinterpreted as bytes). */
int64_t GetVirtualTransactionSize(int64_t nWeight, int64_t nSigOpCost);
int64_t GetVirtualTransactionSize(const CTransaction& tx, int64_t nSigOpCost = 0);
int64_t GetVirtualTransactionInputSize(const CTxIn& tx, int64_t nSigOpCost = 0);

// These implement the weight = (stripped_size * 4) + witness_size formula,
// using only serialization with and without witness data. As witness_size
// is equal to total_size - stripped_size, this formula is identical to:
// weight = (stripped_size * 3) + total_size.
static inline int64_t GetTransactionWeight(const CTransaction& tx)
{
    return GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + GetSerializeSize(tx, SER_NETWORK, PROTOCOL_VERSION);
}
static inline int64_t GetTransactionInputWeight(const CTxIn& txin)
{
    // scriptWitness size is added here because witnesses and txins are split up in segwit serialization.
    return GetSerializeSize(txin, SER_NETWORK, PROTOCOL_VERSION | SERIALIZE_TRANSACTION_NO_WITNESS) * (WITNESS_SCALE_FACTOR - 1) + GetSerializeSize(txin, SER_NETWORK, PROTOCOL_VERSION) + GetSerializeSize(txin.scriptWitness.stack, SER_NETWORK, PROTOCOL_VERSION);
}
}

#endif // BITCOIN_POLICY_POLICY_H
