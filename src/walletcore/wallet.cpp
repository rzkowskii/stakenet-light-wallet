// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2017 The PIVX Developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <wallet.h>

#include <assert.h>
#include <base58.h>
#include <coinselection.h>
#include <fees.h>
#include <key_io.h>
#include <outputtype.h>
#include <policy/policy.h>
#include <random.h>
#include <script/script.h>
#include <script/sign.h>
#include <transaction.h>
#include <utilstrencodings.h>
#include <utiltime.h>
#include <walletdb.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/range/adaptors.hpp>
#include <boost/range/algorithm.hpp>
#include <boost/thread.hpp>

using namespace boost::adaptors;

using namespace std;

namespace bitcoin {

/**
 * Settings
 */
static const size_t OUTPUT_GROUP_MAX_ENTRIES = 10;
static constexpr int DEFAULT_KEYPOOL_SIZE = 10;
static constexpr int DEFAULT_AUX_KEYPOOL_SIZE = 10;
static const std::string PURPOSE_EXTERNAL("external");
static const std::string PURPOSE_INTERNAL("internal");

/**
 * Fees smaller than this (in duffs) are considered zero fee (for transaction creation)
 * We are ~100 times smaller then bitcoin now (2015-06-23), set minTxFee 10 times higher
 * so it's still 10 times lower comparing to bitcoin.
 * Override with -mintxfee
 */
int64_t nStartupTime = GetAdjustedTime();

struct CoinSelectionParams {
    bool use_bnb = true;
    size_t change_output_size = 0;
    size_t change_spend_size = 0;
    CFeeRate effective_fee = CFeeRate(0);
    size_t tx_noinputs_size = 0;

    CoinSelectionParams(bool use_bnb, size_t change_output_size, size_t change_spend_size,
        CFeeRate effective_fee, size_t tx_noinputs_size)
        : use_bnb(use_bnb)
        , change_output_size(change_output_size)
        , change_spend_size(change_spend_size)
        , effective_fee(effective_fee)
        , tx_noinputs_size(tx_noinputs_size)
    {
    }
    CoinSelectionParams() {}
};

/** @defgroup mapWallet
 *
 * @{
 */

static bool IsCurrentForAntiFeeSniping(interfaces::Chain::Lock& locked_chain)
{
    if (locked_chain.isChainSynced()) {
        return false;
    }
    constexpr int64_t MAX_ANTI_FEE_SNIPING_TIP_AGE = 8 * 60 * 60; // in seconds
    if (locked_chain.getBestBlockTime() < (GetTime() - MAX_ANTI_FEE_SNIPING_TIP_AGE)) {
        return false;
    }
    return true;
}

/**
 * Return a height-based locktime for new transactions (uses the height of the
 * current chain tip unless we are not synced with the current chain
 */
static uint32_t GetLocktimeForNewTransaction(interfaces::Chain::Lock& locked_chain)
{
    uint32_t const height = locked_chain.getHeight().get_value_or(-1);
    uint32_t locktime;
    // Discourage fee sniping.
    //
    // For a large miner the value of the transactions in the best block and
    // the mempool can exceed the cost of deliberately attempting to mine two
    // blocks to orphan the current best block. By setting nLockTime such that
    // only the next block can include the transaction, we discourage this
    // practice as the height restricted and limited blocksize gives miners
    // considering fee sniping fewer options for pulling off this attack.
    //
    // A simple way to think about this is from the wallet's point of view we
    // always want the blockchain to move forward. By setting nLockTime this
    // way we're basically making the statement that we only want this
    // transaction to appear in the next block; we don't want to potentially
    // encourage reorgs by allowing transactions to appear at lower heights
    // than the next block in forks of the best chain.
    //
    // Of course, the subsidy is high enough, and transaction volume low
    // enough, that fee sniping isn't a problem yet, but by implementing a fix
    // now we ensure code won't be written that makes assumptions about
    // nLockTime that preclude a fix later.
    if (IsCurrentForAntiFeeSniping(locked_chain)) {
        locktime = height;

        // Secondly occasionally randomly pick a nLockTime even further back, so
        // that transactions that are delayed after signing for whatever reason,
        // e.g. high-latency mix networks and some CoinJoin implementations, have
        // better privacy.
        if (GetRandInt(10) == 0)
            locktime = std::max(0, (int)locktime - GetRandInt(100));
    } else {
        // If our chain is lagging behind, we can't discourage fee sniping nor help
        // the privacy of high-latency transactions. To avoid leaking a potentially
        // unique "nLockTime fingerprint", set nLockTime to a constant.
        locktime = 0;
    }
    assert(locktime <= height);
    assert(locktime < LOCKTIME_THRESHOLD);
    return locktime;
}

CPubKey CWallet::GenerateNewKey(WalletBatch& batch, uint32_t nPurpose, uint32_t nCoinType,
    uint32_t nAccountIndex, bool fInternal)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata

    CKey secret;
    // Create new metadata
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    // use hdchainTmp just to provide seed, but use encrypted chain to save new state.
    CHDChain hdChainTmp;
    if (!GetDecryptedHDChain(hdChainTmp)) {
        throw std::runtime_error(std::string(__func__) + ": GetDecryptedHDChain failed");
    }

    CHDChain hdChainCurrent;
    GetHDChain(hdChainCurrent);
    auto scope = hdChainCurrent.GetChainScope(nPurpose);

    // use HD key derivation if HD was enabled during wallet creation
    DeriveNewChildKey(
        batch, hdChainTmp, metadata, secret, scope, nCoinType, nAccountIndex, fInternal);

    hdChainCurrent.SetChainScope(nPurpose, scope);
    SaveCryptedHDChain(batch, hdChainCurrent);

    return secret.GetPubKey();
}

CPubKey CWallet::GetNewAuxKey(AssetID nCoinType, uint32_t nAccountIndex)
{
    TopUpAuxKeyPool(nCoinType, { nAccountIndex });
    auto& keyPool = _keyPools.at(HD_PURPUSE_LND).at(nCoinType).at(nAccountIndex).externalKeyPool;
    auto it = keyPool.begin();

    if (it == std::end(keyPool)) {
        throw std::runtime_error(std::string(__func__)
            + ": No key found in aux chain for acc number:" + std::to_string(nAccountIndex));
    }

    CPubKey result;
    if (!GetPubKey(nCoinType, it->keyID, result)) {
        throw std::runtime_error(std::string(__func__)
            + ": Failed to get pubkey for acc number:" + std::to_string(nAccountIndex));
    }

    keyPool.erase(it);
    TopUpAuxKeyPool(nCoinType, { nAccountIndex });

    return result;
}

void CWallet::DeriveNewChildKey(WalletBatch& batch, const CHDChain& hdchain,
    const CKeyMetadata& metadata, CKey& secretRet, CHDChainScope& scope, uint32_t nCoinType,
    uint32_t nAccountIndex, bool fInternal)
{
    CExtKey childKey;
    DeriveNewChildKeyHelper(
        metadata, childKey, hdchain, scope, nCoinType, nAccountIndex, fInternal);
    secretRet = childKey.key;

    if (!AddHDPubKey(batch, hdchain, *scope.nPurpose, nCoinType, nAccountIndex, childKey.Neuter(),
            fInternal))
        throw std::runtime_error(std::string(__func__) + ": AddHDPubKey failed");
}

void CWallet::DeriveNewChildKeyHelper(const CKeyMetadata& metadata, CExtKey& childKeyRet,
    const CHDChain& hdChain, CHDChainScope& scope, uint32_t nCoinType, uint32_t nAccountIndex,
    bool fInternal)
{
    CHDAccount acc;
    if (!scope.GetAccount(nCoinType, nAccountIndex, acc))
        throw std::runtime_error(std::string(__func__) + ": Wrong HD account!");

    // derive child key at next index, skip keys already known to the wallet
    CExtKey childKey;
    uint32_t nChildIndex = fInternal ? acc.nInternalChainCounter : acc.nExternalChainCounter;
    do {
        hdChain.DeriveChildExtKey(
            *scope.nPurpose, nCoinType, nAccountIndex, fInternal, nChildIndex, childKey);
        // increment childkey index
        nChildIndex++;
    } while (HaveKey(nCoinType, childKey.key.GetPubKey().GetID()));
    auto secret = childKey.key;
    childKeyRet = childKey;

    CPubKey pubkey = secret.GetPubKey();
    assert(secret.VerifyPubKey(pubkey));

    // store metadata
    mapKeyMetadata[nCoinType][pubkey.GetID()] = metadata;
    UpdateTimeFirstKey(metadata.nCreateTime);

    if (fInternal) {
        acc.nInternalChainCounter = nChildIndex;
    } else {
        acc.nExternalChainCounter = nChildIndex;
    }

    if (!scope.SetAccount(nCoinType, nAccountIndex, acc)) {
        throw std::runtime_error(std::string(__func__) + ": SetAccount failed");
    }
}

void CWallet::TopUpWalletKeyPool(AssetID assetID)
{
    TopUpKeyPoolByAsset(HD_PURPOSE_WALLET_KEYS, assetID, { 0 });
}

CWallet::CWallet(string strWalletDirectory)
{
    SetNull();
    database = WalletDatabase::Create(strWalletDirectory);
}

std::vector<CKeyID> CWallet::GetKeyPoolKeys(AssetID assetID, uint32_t nAccountIndex, bool fInternal)
{
    TopUpWalletKeyPool(assetID);
    std::vector<CKeyID> result;
    const auto& keyPools = _keyPools.at(HD_PURPOSE_WALLET_KEYS).at(assetID).at(nAccountIndex);

    const SetKeyPool& keyPool = fInternal ? keyPools.internalKeyPool : keyPools.externalKeyPool;

    std::transform(std::begin(keyPool), std::end(keyPool), std::back_inserter(result),
        [](const CKeyPool& entry) { return entry.keyID; });

    return result;
}

CPubKey CWallet::GetLastUnusedKey(AssetID nCoinType, uint32_t nAccountIndex, bool fInternal)
{
    TopUpWalletKeyPool(nCoinType);

    const auto& keyPools = _keyPools.at(HD_PURPOSE_WALLET_KEYS).at(nCoinType).at(nAccountIndex);
    const SetKeyPool& keyPool = fInternal ? keyPools.internalKeyPool : keyPools.externalKeyPool;

    const CKeyPool& keyPoolEntry = *keyPool.begin();
    CPubKey result;
    GetPubKey(nCoinType, keyPoolEntry.keyID, result);
    return result;
}

void CWallet::MarkAddressAsUsed(CTxDestination address, CChainParams chainParams, AssetID assetID)
{
    auto keyID = boost::apply_visitor(CTxDestinationToKeyIDVisitor(), address);
    if (!keyID.IsNull()) {
        LOCK(cs_wallet);
        auto it = mapHdPubKeys.find(assetID);
        if (it != std::end(mapHdPubKeys)) {
            const auto& keys = it->second;
            std::map<CKeyID, CHDPubKey>::const_iterator mi = keys.find(keyID);
            if (mi != keys.end()) {
                const CHDPubKey& hdPubKey = mi->second;
                auto& keyPools = _keyPools[HD_PURPOSE_WALLET_KEYS][assetID][0];

                TopUpWalletKeyPool(assetID);

                bool fInternal = hdPubKey.nChangeIndex > 0;

                SetKeyPool& keyPool
                    = fInternal ? keyPools.internalKeyPool : keyPools.externalKeyPool;

                auto it = keyPool.find(
                    CKeyPool(keyID, hdPubKey.nAccountIndex, hdPubKey.extPubKey.nChild, fInternal));

                if (it != std::end(keyPool)) {
                    WalletBatch batch(*database);
                    SetAddressBook(batch, EncodeDestination(address, chainParams), assetID,
                        std::string(), fInternal ? PURPOSE_INTERNAL : PURPOSE_EXTERNAL);
                    keyPool.erase(it);
                    TopUpWalletKeyPool(assetID);
                }
            }
        }
    }
}

void CWallet::RecoverAuxChain(
    std::vector<uint32_t> assets, std::vector<uint32_t> accounts, uint32_t recoveryWindow)
{
    WalletBatch batch(*database);
    std::for_each(std::begin(assets), std::end(assets),
        [&](auto nCoinType) { this->TopUpAuxKeyPool(nCoinType, accounts, recoveryWindow); });
}

CWallet::AddressesList CWallet::GetAddressesByAssetID(AssetID assetID, bool fInternal) const
{
    LOCK(cs_wallet);
    AddressesList result;
    const auto& purpose = fInternal ? PURPOSE_INTERNAL : PURPOSE_EXTERNAL;
    if (mapAddressBook.find(assetID) != mapAddressBook.end()) {
        for (auto pair : mapAddressBook.at(assetID)) {
            if (pair.second.purpose == purpose) {
                result.push_back(pair.first);
            }
        }
    }
    return result;
}

bool CWallet::SelectCoins(const std::vector<COutput>& vAvailableCoins, const CAmount& nTargetValue,
    std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet,
    CoinSelectionParams& coin_selection_params, bool& bnb_used) const
{
    // calculate value from preset inputs and store them
    std::set<CInputCoin> setPresetCoins;
    CAmount nValueFromPresetInputs = 0;

    std::vector<OutputGroup> groups = GroupOutputs(vAvailableCoins, false);

    //    size_t max_ancestors = (size_t)std::max<int64_t>(1, gArgs.GetArg("-limitancestorcount",
    //    DEFAULT_ANCESTOR_LIMIT)); size_t max_descendants = (size_t)std::max<int64_t>(1,
    //    gArgs.GetArg("-limitdescendantcount", DEFAULT_DESCENDANT_LIMIT)); bool fRejectLongChains =
    //    gArgs.GetBoolArg("-walletrejectlongchains", DEFAULT_WALLET_REJECT_LONG_CHAINS);

    bool res = nTargetValue <= nValueFromPresetInputs
        || SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(1, 6, 0),
               groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)
        || SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(1, 1, 0),
               groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used);
    /*||
            (m_spend_zero_conf_change && SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs,
       CoinEligibilityFilter(0, 1, 2), groups, setCoinsRet, nValueRet, coin_selection_params,
       bnb_used)) || (m_spend_zero_conf_change && SelectCoinsMinConf(nTargetValue -
       nValueFromPresetInputs, CoinEligibilityFilter(0, 1, std::min((size_t)4, max_ancestors/3),
       std::min((size_t)4, max_descendants/3)), groups, setCoinsRet, nValueRet,
       coin_selection_params, bnb_used)) || (m_spend_zero_conf_change &&
       SelectCoinsMinConf(nTargetValue - nValueFromPresetInputs, CoinEligibilityFilter(0, 1,
       max_ancestors/2, max_descendants/2), groups, setCoinsRet, nValueRet, coin_selection_params,
       bnb_used)) || (m_spend_zero_conf_change && SelectCoinsMinConf(nTargetValue -
       nValueFromPresetInputs, CoinEligibilityFilter(0, 1, max_ancestors-1, max_descendants-1),
       groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used)) ||
            (m_spend_zero_conf_change && !fRejectLongChains && SelectCoinsMinConf(nTargetValue -
       nValueFromPresetInputs, CoinEligibilityFilter(0, 1, std::numeric_limits<uint64_t>::max()),
       groups, setCoinsRet, nValueRet, coin_selection_params, bnb_used));*/

    // because SelectCoinsMinConf clears the setCoinsRet, we now add the possible inputs to the
    // coinset
    setCoinsRet.insert(setPresetCoins.begin(), setPresetCoins.end());

    // add preset inputs to the total value selected
    nValueRet += nValueFromPresetInputs;

    return res;
}

bool CWallet::SelectCoinsMinConf(const CAmount& nTargetValue,
    const CoinEligibilityFilter& eligibility_filter, std::vector<OutputGroup> groups,
    std::set<CInputCoin>& setCoinsRet, CAmount& nValueRet,
    const CoinSelectionParams& coin_selection_params, bool& bnb_used) const
{
    setCoinsRet.clear();
    nValueRet = 0;

    std::vector<OutputGroup> utxo_pool;
    if (coin_selection_params.use_bnb) {
        // Get long term estimate
        CFeeRate long_term_feerate = GetMinimumFeeRate(*this);

        // Calculate cost of change
        CAmount cost_of_change = long_term_feerate.GetFee(
            coin_selection_params
                .change_spend_size); // +
                                     // coin_selection_params.effective_fee.GetFee(coin_selection_params.change_output_size);

        // Filter by the min conf specs and add to utxo_pool and calculate effective value
        for (OutputGroup& group : groups) {
            if (!group.EligibleForSpending(eligibility_filter))
                continue;

            group.fee = 0;
            group.long_term_fee = 0;
            group.effective_value = 0;
            for (auto it = group.m_outputs.begin(); it != group.m_outputs.end();) {
                const CInputCoin& coin = *it;
                CAmount effective_value = coin.txout.nValue
                    - (coin.m_input_bytes < 0
                              ? 0
                              : coin_selection_params.effective_fee.GetFee(coin.m_input_bytes));
                // Only include outputs that are positive effective value (i.e. not dust)
                if (effective_value > 0) {
                    group.fee += coin.m_input_bytes < 0
                        ? 0
                        : coin_selection_params.effective_fee.GetFee(coin.m_input_bytes);
                    group.long_term_fee += coin.m_input_bytes < 0
                        ? 0
                        : long_term_feerate.GetFee(coin.m_input_bytes);
                    group.effective_value += effective_value;
                    ++it;
                } else {
                    it = group.Discard(coin);
                }
            }
            if (group.effective_value > 0)
                utxo_pool.push_back(group);
        }
        // Calculate the fees for things that aren't inputs
        CAmount not_input_fees
            = coin_selection_params.effective_fee.GetFee(coin_selection_params.tx_noinputs_size);
        bnb_used = true;
        return SelectCoinsBnB(
            utxo_pool, nTargetValue, cost_of_change, setCoinsRet, nValueRet, not_input_fees);
    } else {
        // Filter by the min conf specs and add to utxo_pool
        for (const OutputGroup& group : groups) {
            if (!group.EligibleForSpending(eligibility_filter))
                continue;
            utxo_pool.push_back(group);
        }
        bnb_used = false;
        return KnapsackSolver(nTargetValue, utxo_pool, setCoinsRet, nValueRet);
    }
}

std::vector<OutputGroup> CWallet::GroupOutputs(
    const std::vector<COutput>& outputs, bool single_coin) const
{
    std::vector<OutputGroup> groups;
    std::map<CTxDestination, OutputGroup> gmap;
    CTxDestination dst;
    for (const auto& output : outputs) {
        if (true /*output.fSpendable*/) {
            CInputCoin input_coin = output.GetInputCoin();

            size_t ancestors = 0;
            size_t descendants = 0;
            if (!single_coin && ExtractDestination(input_coin.txout.scriptPubKey, dst)) {
                // Limit output groups to no more than 10 entries, to protect
                // against inadvertently creating a too-large transaction
                // when using -avoidpartialspends
                if (gmap[dst].m_outputs.size() >= OUTPUT_GROUP_MAX_ENTRIES) {
                    groups.push_back(gmap[dst]);
                    gmap.erase(dst);
                }
                gmap[dst].Insert(input_coin, output.nDepth, true, ancestors, descendants);
            } else {
                groups.emplace_back(input_coin, output.nDepth, true, ancestors, descendants);
            }
        }
    }
    if (!single_coin) {
        for (const auto& it : gmap)
            groups.push_back(it.second);
    }
    return groups;
}

bool CWallet::GetPubKey(uint32_t nCoinType, const CKeyID& address, CPubKey& vchPubKeyOut) const
{
    LOCK(cs_wallet);
    auto it = mapHdPubKeys.find(nCoinType);
    if (it != std::end(mapHdPubKeys)) {
        const auto& keys = it->second;
        std::map<CKeyID, CHDPubKey>::const_iterator mi = keys.find(address);
        if (mi != keys.end()) {
            const CHDPubKey& hdPubKey = (*mi).second;
            vchPubKeyOut = hdPubKey.extPubKey.pubkey;
            return true;
        }
    }

    return false;
}

bool CWallet::GetKey(uint32_t nCoinType, const CKeyID& address, CKey& keyOut) const
{
    LOCK(cs_wallet);
    auto it = mapHdPubKeys.find(nCoinType);
    if (it != std::end(mapHdPubKeys)) {
        const auto& keys = it->second;
        std::map<CKeyID, CHDPubKey>::const_iterator mi = keys.find(address);
        if (mi != keys.end()) {
            // if the key has been found in mapHdPubKeys, derive it on the fly
            const CHDPubKey& hdPubKey = (*mi).second;
            CHDChain hdChainCurrent;
            if (!GetDecryptedHDChain(hdChainCurrent)) {
                throw std::runtime_error(std::string(__func__) + ": GetDecryptedHDChain failed");
            }

            CExtKey extkey;
            hdChainCurrent.DeriveChildExtKey(hdPubKey.nPurpose, hdPubKey.nCoinType,
                hdPubKey.nAccountIndex, hdPubKey.nChangeIndex != 0, hdPubKey.extPubKey.nChild,
                extkey);
            keyOut = extkey.key;

            return true;
        }
    }

    return false;
}

bool CWallet::HaveKey(uint32_t nCoinType, const CKeyID& address) const
{
    LOCK(cs_wallet);
    return mapHdPubKeys.count(nCoinType) && mapHdPubKeys.at(nCoinType).count(address) > 0;
}

bool CWallet::AddHDPubKey(WalletBatch& batch, const CHDChain& hdChain, uint32_t nPurpose,
    uint32_t nCoinType, uint32_t nAccountIndex, const CExtPubKey& extPubKey, bool fInternal)
{
    AssertLockHeld(cs_wallet);

    CHDPubKey hdPubKey;
    hdPubKey.extPubKey = extPubKey;
    hdPubKey.hdchainID = hdChain.GetID();
    hdPubKey.nCoinType = nCoinType;
    hdPubKey.nChangeIndex = fInternal ? 1 : 0;
    hdPubKey.nAccountIndex = nAccountIndex;
    hdPubKey.nPurpose = nPurpose;
    LoadHDPubKey(nCoinType, hdPubKey);

    // check if we need to remove from watch-only
    CScript script;
    script = GetScriptForDestination(extPubKey.pubkey.GetID());

    return batch.WriteHDPubKey(
        nCoinType, hdPubKey, mapKeyMetadata[nCoinType][extPubKey.pubkey.GetID()]);
}

bool CWallet::LoadKeyMetadata(uint32_t nCoinType, const CKeyID& keyID, const CKeyMetadata& meta)
{
    AssertLockHeld(cs_wallet); // mapKeyMetadata
    if (meta.nCreateTime && (!nTimeFirstKey || meta.nCreateTime < nTimeFirstKey))
        nTimeFirstKey = meta.nCreateTime;

    mapKeyMetadata[nCoinType][keyID] = meta;
    return true;
}

bool CWallet::FindHDPubKey(
    uint32_t nCoinType, uint32_t nAccountIndex, bool internal, uint32_t nIndex, CPubKey& pubkey)
{
    try {
        auto& where = mapHdPubKeys.at(nCoinType);
        auto it = std::find_if(std::begin(where), std::end(where), [&](const auto& it) {
            const CHDPubKey& pubkey = it.second;
            return pubkey.nPurpose == HD_PURPOSE_WALLET_KEYS && pubkey.nCoinType == nCoinType
                && pubkey.nAccountIndex == nAccountIndex
                && pubkey.nChangeIndex == (internal ? 1 : 0) && pubkey.extPubKey.nChild == nIndex;
        });

        if (it != std::end(where)) {
            pubkey = it->second.extPubKey.pubkey;
            return true;
        }

    } catch (std::exception&) {
    }

    return false;
}

bool CWallet::LoadMinVersion(int nVersion)
{
    AssertLockHeld(cs_wallet);
    nWalletVersion = nVersion;
    nWalletMaxVersion = std::max(nWalletMaxVersion, nVersion);
    return true;
}

void CWallet::UpdateTimeFirstKey(int64_t nCreateTime)
{
    AssertLockHeld(cs_wallet);
    if (nCreateTime <= 1) {
        // Cannot determine birthday information, so set the wallet birthday to
        // the beginning of time.
        nTimeFirstKey = 1;
    } else if (!nTimeFirstKey || nCreateTime < nTimeFirstKey) {
        nTimeFirstKey = nCreateTime;
    }
}

void CWallet::TopUpKeyPool(size_t kpSize)
{
    {
        LOCK(cs_wallet);

        // Top up key pool
        size_t nTargetSize = kpSize > 0 ? kpSize : DEFAULT_KEYPOOL_SIZE;

        for (auto&& keyPoolsIt : _keyPools) {
            auto assetID = keyPoolsIt.first;
            TopUpKeyPoolByAsset(HD_PURPOSE_WALLET_KEYS, assetID, { 0 }, nTargetSize, nTargetSize);
        }
    }
}

void CWallet::TopUpKeyPoolByAsset(uint32_t nPurpose, AssetID assetID,
    std::vector<uint32_t> accounts, std::optional<int> internalSize,
    std::optional<int> externalSize)
{
    // Top up key pool
    size_t nTargetSizeInternal = internalSize.value_or(DEFAULT_KEYPOOL_SIZE);
    size_t nTargetSizeExternal = externalSize.value_or(DEFAULT_KEYPOOL_SIZE);

    auto& assetKeyPools = _keyPools[nPurpose][assetID];

    CHDChain hdChainTmp;
    if (!GetDecryptedHDChain(hdChainTmp)) {
        throw std::runtime_error(std::string(__func__) + ": GetDecryptedHDChain failed");
    }
    // Create new metadata
    int64_t nCreationTime = GetTime();
    CKeyMetadata metadata(nCreationTime);

    CHDChain hdChainCurrent;
    GetHDChain(hdChainCurrent);
    auto scope = hdChainCurrent.GetChainScope(nPurpose);

    WalletBatch batch(*database);
    bool generated = false;
    for (auto&& nFamilyIndex : accounts) {

        CHDAccount acc;
        while (!scope.GetAccount(assetID, nFamilyIndex, acc)) {
            scope.AddAccount(assetID);
        }

        hdChainCurrent.SetChainScope(nPurpose, scope);

        auto& keyPools = assetKeyPools[nFamilyIndex];

        // count amount of available keys (internal, external)
        // make sure the keypool of external and internal keys fits the user selected target
        // (-keypool)
        size_t missingExternal = nTargetSizeExternal > keyPools.externalKeyPool.size()
            ? nTargetSizeExternal - keyPools.externalKeyPool.size()
            : 0;
        size_t missingInternal = nTargetSizeInternal > keyPools.internalKeyPool.size()
            ? nTargetSizeInternal - keyPools.internalKeyPool.size()
            : 0;

        for (size_t i = 0; i < missingExternal; ++i) {
            CKey childKey;
            DeriveNewChildKey(
                batch, hdChainTmp, metadata, childKey, scope, assetID, nFamilyIndex, false);
            auto keyID = childKey.GetPubKey().GetID();
            CHDPubKey hdPubKey = mapHdPubKeys.at(assetID).at(keyID);
            keyPools.externalKeyPool.insert(
                CKeyPool(keyID, hdPubKey.nAccountIndex, hdPubKey.extPubKey.nChild, false));
        }

        for (size_t i = 0; i < missingInternal; ++i) {
            CKey childKey;
            DeriveNewChildKey(
                batch, hdChainTmp, metadata, childKey, scope, assetID, nFamilyIndex, true);
            auto keyID = childKey.GetPubKey().GetID();
            CHDPubKey hdPubKey = mapHdPubKeys.at(assetID).at(keyID);
            keyPools.internalKeyPool.insert(
                CKeyPool(keyID, hdPubKey.nAccountIndex, hdPubKey.extPubKey.nChild, true));
        }

        generated |= ((missingInternal + missingExternal) > 0);
    }

    if (generated) {
        batch.WritePool(nPurpose, assetID, assetKeyPools);
        hdChainCurrent.SetChainScope(nPurpose, scope);

        SaveCryptedHDChain(batch, hdChainCurrent);
    }
}

void CWallet::TopUpAuxKeyPool(AssetID assetID, std::vector<uint32_t> accounts, size_t kpSize)
{
    auto size = kpSize > 0 ? std::make_optional(kpSize) : std::nullopt;
    TopUpKeyPoolByAsset(HD_PURPUSE_LND, assetID, accounts, 0, size);
}

void CWallet::DeriveIdentityPubKey()
{
    CHDChain mainHDChain;
    if (GetDecryptedHDChain(mainHDChain)) {
        CExtKey key;
        mainHDChain.DeriveChildExtKey(HD_PURPUSE_UTILITY, 0, 0, false, 0, key);
        identityPubKey = key.Neuter().pubkey.GetID().ToString();
    }
}

void CWallet::SaveCryptedHDChain(WalletBatch& batch, CHDChain hdChainCurrent)
{
    if (IsCrypted()) {
        if (!SetCryptedHDChain(&batch, hdChainCurrent, false))
            throw std::runtime_error(std::string(__func__) + ": SetCryptedHDChain failed");
    } else {
        if (!SetHDChain(&batch, hdChainCurrent, false))
            throw std::runtime_error(std::string(__func__) + ": SetHDChain failed");
    }
}

#if 0
bool CWallet::AddCScript(uint32_t nCoinType, const CScript& redeemScript)
{
    if (!CCryptoKeyStore::AddCScript(nCoinType, redeemScript))
        return false;

    return WalletBatch(*database).WriteCScript(nCoinType, Hash160(redeemScript), redeemScript);
}

bool CWallet::LoadCScript(uint32_t nCoinType, const CScript& redeemScript)
{
    /* A sanity check was added in pull #3843 to avoid adding redeemScripts
     * that never can be redeemed. However, old wallets may still contain
     * these. Do not add them to the wallet and warn. */
    if (redeemScript.size() > MAX_SCRIPT_ELEMENT_SIZE) {
        //        std::string strAddr = CBitcoinAddress(CScriptID(redeemScript)).ToString();
        //        LogPrintf("%s: Warning: This wallet contains a redeemScript of size %i which
        //        exceeds maximum size %i thus can never be redeemed. Do not use address %s.\n",
        //                  __func__, redeemScript.size(), MAX_SCRIPT_ELEMENT_SIZE, strAddr);
        return true;
    }

    return CCryptoKeyStore::AddCScript(nCoinType, redeemScript);
}
#endif

bool CWallet::Unlock(const SecureString& strWalletPassphrase)
{
    SecureString strWalletPassphraseFinal;

    if (!IsLocked()) {
        return true;
    }

    strWalletPassphraseFinal = strWalletPassphrase;

    CCrypter crypter;
    CKeyingMaterial vMasterKey;

    {
        LOCK(cs_wallet);
        for (const MasterKeyMap::value_type& pMasterKey : mapMasterKeys) {
            if (!crypter.SetKeyFromPassphrase(strWalletPassphraseFinal, pMasterKey.second.vchSalt,
                    pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                continue; // try another master key

            if (CCryptoKeyStore::Unlock(vMasterKey)) {
                DeriveIdentityPubKey();
                return true;
            }
        }
    }
    return false;
}

bool CWallet::ChangeWalletPassphrase(
    const SecureString& strOldWalletPassphrase, const SecureString& strNewWalletPassphrase)
{
    throw std::runtime_error("Not implemented");
#if 0
    bool fWasLocked = IsLocked();
    SecureString strOldWalletPassphraseFinal = strOldWalletPassphrase;

    {
        LOCK(cs_wallet);
        Lock();

        CCrypter crypter;
        CKeyingMaterial vMasterKey;
        for (MasterKeyMap::value_type& pMasterKey : mapMasterKeys) {
            if (!crypter.SetKeyFromPassphrase(strOldWalletPassphraseFinal, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                return false;
            if (!crypter.Decrypt(pMasterKey.second.vchCryptedKey, vMasterKey))
                return false;
            if (CCryptoKeyStore::Unlock(vMasterKey)) {
                int64_t nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = pMasterKey.second.nDeriveIterations * (100 / ((double)(GetTimeMillis() - nStartTime)));

                nStartTime = GetTimeMillis();
                crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod);
                pMasterKey.second.nDeriveIterations = (pMasterKey.second.nDeriveIterations + pMasterKey.second.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime))) / 2;

                if (pMasterKey.second.nDeriveIterations < 25000)
                    pMasterKey.second.nDeriveIterations = 25000;

                if (!crypter.SetKeyFromPassphrase(strNewWalletPassphrase, pMasterKey.second.vchSalt, pMasterKey.second.nDeriveIterations, pMasterKey.second.nDerivationMethod))
                    return false;
                if (!crypter.Encrypt(vMasterKey, pMasterKey.second.vchCryptedKey))
                    return false;
                CWalletDB(strWalletFile, strDatabaseEnvPath).WriteMasterKey(pMasterKey.first, pMasterKey.second);
                if (fWasLocked)
                    Lock();

                return true;
            }
        }
    }

    return false;
#endif
}

bool CWallet::SetMinVersion(enum WalletFeature nVersion, WalletBatch* batch_in, bool fExplicit)
{
    LOCK(cs_wallet); // nWalletVersion
    if (nWalletVersion >= nVersion)
        return true;

    // when doing an explicit upgrade, if we pass the max version permitted, upgrade all the way
    if (fExplicit && nVersion > nWalletMaxVersion)
        nVersion = FEATURE_LATEST;

    nWalletVersion = nVersion;

    if (nVersion > nWalletMaxVersion)
        nWalletMaxVersion = nVersion;

    WalletBatch* batch = batch_in ? batch_in : new WalletBatch(*database);
    if (nWalletVersion > 40000)
        batch->WriteMinVersion(nWalletVersion);
    if (!batch_in)
        delete batch;

    return true;
}

bool CWallet::SetMaxVersion(int nVersion)
{
    LOCK(cs_wallet); // nWalletVersion, nWalletMaxVersion
    // cannot downgrade below current version
    if (nWalletVersion > nVersion)
        return false;

    nWalletMaxVersion = nVersion;

    return true;
}

string CWallet::GetAddressType(AssetID assetID) const
{
    LOCK(cs_wallet);
    return addressTypes.count(assetID) > 0 ? addressTypes.at(assetID) : std::string();
}

void CWallet::SetAddressType(AssetID assetID, std::string type)
{
    LOCK(cs_wallet);
    if (addressTypes.count(assetID) == 0) {
        addressTypes[assetID] = type;
        WalletBatch(*database).WriteAddressType(addressTypes);
    }
}

bool CWallet::EncryptWallet(const SecureString& strWalletPassphrase)
{
    if (IsCrypted())
        return false;

    CKeyingMaterial vMasterKey;

    vMasterKey.resize(WALLET_CRYPTO_KEY_SIZE);
    GetStrongRandBytes(&vMasterKey[0], WALLET_CRYPTO_KEY_SIZE);

    CMasterKey kMasterKey;

    kMasterKey.vchSalt.resize(WALLET_CRYPTO_SALT_SIZE);
    GetStrongRandBytes(&kMasterKey.vchSalt[0], WALLET_CRYPTO_SALT_SIZE);

    CCrypter crypter;
    int64_t nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(
        strWalletPassphrase, kMasterKey.vchSalt, 25000, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations = 2500000 / ((double)(GetTimeMillis() - nStartTime));

    nStartTime = GetTimeMillis();
    crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt,
        kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod);
    kMasterKey.nDeriveIterations
        = (kMasterKey.nDeriveIterations
              + kMasterKey.nDeriveIterations * 100 / ((double)(GetTimeMillis() - nStartTime)))
        / 2;

    if (kMasterKey.nDeriveIterations < 25000)
        kMasterKey.nDeriveIterations = 25000;

    //    LogPrintf("Encrypting Wallet with an nDeriveIterations of %i\n",
    //    kMasterKey.nDeriveIterations);

    if (!crypter.SetKeyFromPassphrase(strWalletPassphrase, kMasterKey.vchSalt,
            kMasterKey.nDeriveIterations, kMasterKey.nDerivationMethod))
        return false;
    if (!crypter.Encrypt(vMasterKey, kMasterKey.vchCryptedKey))
        return false;

    {
        LOCK(cs_wallet);
        mapMasterKeys[++nMasterKeyMaxID] = kMasterKey;
        WalletBatch* encrypted_batch = new WalletBatch(*database);
        if (!encrypted_batch->TxnBegin()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            return false;
        }
        encrypted_batch->WriteMasterKey(nMasterKeyMaxID, kMasterKey);

        // must get current HD chain before EncryptKeys
        CHDChain hdChainCurrent;
        if (!GetHDChain(hdChainCurrent)) {
            assert(false);
            return false;
        }

        if (!EncryptHDChain(vMasterKey)) {
            assert(false);
            return false;
        }

        CHDChain hdChainCrypted;
        if (!GetHDChain(hdChainCrypted)) {
            assert(false);
            //            Q_ASSERT_X(false, __FUNCTION__, "No crypted HD chain");
            return false;
        }

        // ids should match, seed hashes should not
        assert(hdChainCurrent.GetID() == hdChainCrypted.GetID());
        assert(hdChainCurrent.GetSeedHash() != hdChainCrypted.GetSeedHash());

        if (!SetCryptedHDChain(encrypted_batch, hdChainCrypted, false)) {
            return false;
        }

        // Encryption was introduced in version 0.4.0
        SetMinVersion(FEATURE_WALLETCRYPT, encrypted_batch, true);

        if (!encrypted_batch->TxnCommit()) {
            delete encrypted_batch;
            encrypted_batch = nullptr;
            // We now have keys encrypted in memory, but not on disk...
            // die to avoid confusion and let the user reload the unencrypted wallet.
            assert(false);
            exit(1);
        }

        delete encrypted_batch;
        encrypted_batch = nullptr;

        Lock();
        Unlock(strWalletPassphrase);

        // Need to completely rewrite the wallet file; if we don't, bdb might keep
        // bits of the unencrypted private key in slack space in the database file.
        database->Rewrite();
        database->ReloadDbEnv();
    }
    NotifyStatusChanged(this);

    return true;
}

bool CWallet::IsEncrypted()
{
    bitcoin::WalletBatch batch(GetDBHandle());
    return batch.HasMasterKey();
}

std::unique_ptr<SigningProvider> CWallet::CreateSignatureProvider(uint32_t nCoinType) const
{
    struct Provider : public SigningProvider {
        explicit Provider(const CWallet& wallet, uint32_t nCoinType)
            : _wallet(wallet)
            , _nCoinType(nCoinType)
        {
        }

        virtual bool GetCScript(const CScriptID& scriptid, CScript& script) const override
        {
            throw std::runtime_error(__func__ + std::string{ "Not implemented" });
            //            return _wallet.GetCScript(_nCoinType, scriptid, script);
        }
        virtual bool GetPubKey(const CKeyID& address, CPubKey& pubkey) const override
        {
            return _wallet.GetPubKey(_nCoinType, address, pubkey);
        }
        virtual bool GetKey(const CKeyID& address, CKey& key) const override
        {
            return _wallet.GetKey(_nCoinType, address, key);
        }
        virtual bool GetKeyOrigin(const CKeyID& keyid, KeyOriginInfo& info) const override
        {
            return false;
        }

        const CWallet& _wallet;
        uint32_t _nCoinType;
    };

    return std::unique_ptr<SigningProvider>(new Provider(*this, nCoinType));
}

static OutputType TransactionChangeType(const std::vector<CRecipient>& vecSend)
{
    // TODO: hardcoded bech32 change type
    return OutputType::BECH32;

    // if any destination is P2WPKH or P2WSH, use P2WPKH for the change
    // output.
    for (const auto& recipient : vecSend) {
        // Check if any destination contains a witness program:
        int witnessversion = 0;
        std::vector<unsigned char> witnessprogram;
        if (recipient.scriptPubKey.IsWitnessProgram(witnessversion, witnessprogram)) {
            return OutputType::BECH32;
        }
    }

    return OutputType::LEGACY;
}

bool CWallet::CreateTransaction(interfaces::Chain::Lock& locked_chain,
    const interfaces::CoinsView& coinsView, interfaces::ReserveKeySource& reserveKeySource,
    uint32_t nCoinType, const std::vector<CRecipient>& vecSend, CTransactionRef& tx,
    boost::optional<CFeeRate> feeRate, CAmount& nFeeRet, int& nChangePosInOut,
    std::string& strFailReason)
{
    CAmount nValue = 0;
    int nChangePosRequest = nChangePosInOut;
    unsigned int nSubtractFeeFromAmount = 0;
    for (const auto& recipient : vecSend) {
        if (nValue < 0 || recipient.nAmount < 0) {
            strFailReason = "Transaction amounts must not be negative";
            return false;
        }
        nValue += recipient.nAmount;

        if (recipient.fSubtractFeeFromAmount)
            nSubtractFeeFromAmount++;
    }
    if (vecSend.empty()) {
        strFailReason = "Transaction must have at least one recipient";
        return false;
    }

    CMutableTransaction txNew;

    txNew.nLockTime = GetLocktimeForNewTransaction(locked_chain);

    auto signingProvider = CreateSignatureProvider(nCoinType);

    CAmount nFeeNeeded;
    int nBytes;
    {
        std::set<CInputCoin> setCoins;
        LOCK(cs_wallet);
        {
            auto vAvailableCoins = coinsView.availableCoins(locked_chain, false);

            CoinSelectionParams
                coin_selection_params; // Parameters for coin selection, init with dummy

            // Create change script that will be used if we need change
            // TODO: pass in scriptChange instead of reservekey so
            // change transaction isn't always pay-to-bitcoin-address
            CScript scriptChange;

            /*// coin control: send change to custom address
            if (!boost::get<CNoDestination>(&coin_control.destChange)) {
                scriptChange = GetScriptForDestination(coin_control.destChange);
            } else */
            { // no coin control: send change to newly generated address
                // Note: We use a new key here to keep it from being obvious which side is the
                // change.
                //  The drawback is that by not reusing a previous key, the change may be lost if a
                //  backup is restored, if the backup doesn't have the new private key for the
                //  change. If we reused the old key, it would be possible to add code to look for
                //  and rediscover unknown transactions that were written with keys of ours to
                //  recover post-backup change.

                // Reserve a new key pair from key pool
                auto changeAddress = reserveKeySource.getReservedKey();
                if (!changeAddress) {
                    strFailReason = "Can't generate a change-address key. No keys in the internal "
                                    "keypool and can't generate any keys.";
                    return false;
                }
                CPubKey vchPubKey = *changeAddress;

                //                const OutputType change_type =
                //                TransactionChangeType(coin_control.m_change_type ?
                //                *coin_control.m_change_type : m_default_change_type, vecSend);

                //                LearnRelatedScripts(vchPubKey, change_type);
                //                                scriptChange =
                //                                GetScriptForDestination(GetDestinationForKey(vchPubKey,
                //                                change_type));
                scriptChange = GetScriptForDestination(
                    GetDestinationForKey(vchPubKey.GetID(), TransactionChangeType(vecSend)));
            }
            CTxOut change_prototype_txout(0, scriptChange);
            coin_selection_params.change_output_size
                = GetSerializeSize(change_prototype_txout, SER_NETWORK, PROTOCOL_VERSION);

            // Get the fee rate to use effective values in coin selection
            CFeeRate nFeeRateNeeded = GetMinimumFeeRate(*this);

            nFeeRet = 0;
            bool pick_new_inputs = true;
            CAmount nValueIn = 0;

            // BnB selector is the only selector used when this is true.
            // That should only happen on the first pass through the loop.
            coin_selection_params.use_bnb = nSubtractFeeFromAmount
                == 0; // If we are doing subtract fee from recipient, then don't use BnB
            // Start with no fee and loop until there is enough fee
            while (true) {
                nChangePosInOut = nChangePosRequest;
                txNew.vin.clear();
                txNew.vout.clear();
                bool fFirst = true;

                CAmount nValueToSelect = nValue;
                if (nSubtractFeeFromAmount == 0)
                    nValueToSelect += nFeeRet;

                // vouts to the payees
                coin_selection_params.tx_noinputs_size
                    = 11; // Static vsize overhead + outputs vsize. 4 nVersion, 4 nLocktime, 1 input
                          // count, 1 output count, 1 witness overhead (dummy, flag, stack size)
                for (const auto& recipient : vecSend) {
                    CTxOut txout(recipient.nAmount, recipient.scriptPubKey);

                    if (recipient.fSubtractFeeFromAmount) {
                        assert(nSubtractFeeFromAmount != 0);
                        txout.nValue
                            -= nFeeRet / nSubtractFeeFromAmount; // Subtract fee equally from each
                                                                 // selected recipient

                        if (fFirst) // first receiver pays the remainder not divisible by output
                                    // count
                        {
                            fFirst = false;
                            txout.nValue -= nFeeRet % nSubtractFeeFromAmount;
                        }
                    }
                    // Include the fee cost for outputs. Note this is only used for BnB right now
                    coin_selection_params.tx_noinputs_size
                        += GetSerializeSize(txout, SER_NETWORK, PROTOCOL_VERSION);

                    if (IsDust(txout, dustRelayFee)) {
                        if (recipient.fSubtractFeeFromAmount && nFeeRet > 0) {
                            if (txout.nValue < 0)
                                strFailReason
                                    = "The transaction amount is too small to pay the fee";
                            else
                                strFailReason = "The transaction amount is too small to send after "
                                                "the fee has been deducted";
                        } else
                            strFailReason = "Transaction amount too small";
                        return false;
                    }
                    txNew.vout.push_back(txout);
                }

                // Choose coins to use
                bool bnb_used;
                if (pick_new_inputs) {
                    nValueIn = 0;
                    setCoins.clear();
                    coin_selection_params.change_spend_size
                        = CalculateMaximumSignedInputSize(*signingProvider, change_prototype_txout);
                    // If the wallet doesn't know how to sign change output, assume p2sh-p2wpkh
                    // as lower-bound to allow BnB to do it's thing
                    coin_selection_params.effective_fee = nFeeRateNeeded;
                    coin_selection_params.use_bnb = false;
                    if (!SelectCoins(vAvailableCoins, nValueToSelect, setCoins, nValueIn,
                            coin_selection_params, bnb_used)) {
                        // If BnB was used, it was the first pass. No longer the first pass and
                        // continue loop with knapsack.
                        if (bnb_used) {
                            coin_selection_params.use_bnb = false;
                            continue;
                        } else {
                            strFailReason = "Insufficient funds";
                            return false;
                        }
                    }
                } else {
                    bnb_used = false;
                }

                const CAmount nChange = nValueIn - nValueToSelect;
                if (nChange > 0) {
                    // Fill a vout to ourself
                    CTxOut newTxOut(nChange, scriptChange);

                    // Never create dust outputs; if we would, just
                    // add the dust to the fee.
                    // The nChange when BnB is used is always going to go to fees.
                    if (IsDust(newTxOut, dustRelayFee) || bnb_used) {
                        nChangePosInOut = -1;
                        nFeeRet += nChange;
                    } else {
                        if (nChangePosInOut == -1) {
                            // Insert change txn at random position:
                            nChangePosInOut = GetRandInt(txNew.vout.size() + 1);
                        } else if ((unsigned int)nChangePosInOut > txNew.vout.size()) {
                            strFailReason = "Change index out of range";
                            return false;
                        }

                        std::vector<CTxOut>::iterator position
                            = txNew.vout.begin() + nChangePosInOut;
                        txNew.vout.insert(position, newTxOut);
                    }
                } else {
                    nChangePosInOut = -1;
                }

                // Dummy fill vin for maximum size estimation
                //
                for (const auto& coin : setCoins) {
                    txNew.vin.push_back(CTxIn(coin.outpoint, CScript()));
                }

                nBytes = CalculateMaximumSignedTxSize(
                    *signingProvider, locked_chain, coinsView, CTransaction(txNew));
                if (nBytes < 0) {
                    strFailReason = "Signing transaction failed";
                    return false;
                }

                nFeeNeeded = feeRate.get_value_or(GetMinimumFeeRate(*this)).GetFee(nBytes);

                // If we made it here and we aren't even able to meet the relay fee on the next
                // pass, give up because we must be at the maximum allowed fee.
                if (nFeeNeeded < GetMinimumFeeRate(*this).GetFee(nBytes)) {
                    strFailReason = "Transaction too large for fee policy";
                    return false;
                }

                if (nFeeRet >= nFeeNeeded) {
                    // Reduce fee to only the needed amount if possible. This
                    // prevents potential overpayment in fees if the coins
                    // selected to meet nFeeNeeded result in a transaction that
                    // requires less fee than the prior iteration.

                    // If we have no change and a big enough excess fee, then
                    // try to construct transaction again only without picking
                    // new inputs. We now know we only need the smaller fee
                    // (because of reduced tx size) and so we should add a
                    // change output. Only try this once.
                    if (nChangePosInOut == -1 && nSubtractFeeFromAmount == 0 && pick_new_inputs) {
                        unsigned int tx_size_with_change = nBytes
                            + coin_selection_params.change_output_size
                            + 2; // Add 2 as a buffer in case increasing # of outputs changes
                                 // compact size
                        CAmount fee_needed_with_change = GetMinimumFee(*this, tx_size_with_change);
                        CAmount minimum_value_for_change
                            = GetDustThreshold(change_prototype_txout, dustRelayFee);
                        if (nFeeRet >= fee_needed_with_change + minimum_value_for_change) {
                            pick_new_inputs = false;
                            nFeeRet = fee_needed_with_change;
                            continue;
                        }
                    }

                    // If we have change output already, just increase it
                    if (nFeeRet > nFeeNeeded && nChangePosInOut != -1
                        && nSubtractFeeFromAmount == 0) {
                        CAmount extraFeePaid = nFeeRet - nFeeNeeded;
                        std::vector<CTxOut>::iterator change_position
                            = txNew.vout.begin() + nChangePosInOut;
                        change_position->nValue += extraFeePaid;
                        nFeeRet -= extraFeePaid;
                    }
                    break; // Done, enough fee included.
                } else if (!pick_new_inputs) {
                    // This shouldn't happen, we should have had enough excess
                    // fee to pay for the new output and still meet nFeeNeeded
                    // Or we should have just subtracted fee from recipients and
                    // nFeeNeeded should not have changed
                    strFailReason = "Transaction fee and change calculation failed";
                    return false;
                }

                // Try to reduce change to include necessary fee
                if (nChangePosInOut != -1 && nSubtractFeeFromAmount == 0) {
                    CAmount additionalFeeNeeded = nFeeNeeded - nFeeRet;
                    std::vector<CTxOut>::iterator change_position
                        = txNew.vout.begin() + nChangePosInOut;
                    // Only reduce change if remaining amount is still a large enough output.
                    if (change_position->nValue >= MIN_FINAL_CHANGE + additionalFeeNeeded) {
                        change_position->nValue -= additionalFeeNeeded;
                        nFeeRet += additionalFeeNeeded;
                        break; // Done, able to increase fee from change
                    }
                }

                // If subtracting fee from recipients, we now know what fee we
                // need to subtract, we have no reason to reselect inputs
                if (nSubtractFeeFromAmount > 0) {
                    pick_new_inputs = false;
                }

                // Include more fee and try again.
                nFeeRet = nFeeNeeded;
                coin_selection_params.use_bnb = false;
                continue;
            }
        }

        //        if (nChangePosInOut == -1) reservekey.ReturnKey(); // Return any reserved key if
        //        we don't have change

        // Shuffle selected coins and fill in final vin
        txNew.vin.clear();
        std::vector<CInputCoin> selected_coins(setCoins.begin(), setCoins.end());
        Shuffle(selected_coins.begin(), selected_coins.end(), FastRandomContext());

        // Note how the sequence number is set to non-maxint so that
        // the nLockTime set above actually works.
        //
        // BIP125 defines opt-in RBF as any nSequence < maxint-1, so
        // we use the highest possible value in that range (maxint-2)
        // to avoid conflicting with other possible uses of nSequence,
        // and in the spirit of "smallest possible change from prior
        // behavior."
        const uint32_t nSequence = CTxIn::SEQUENCE_FINAL - 1;
        for (const auto& coin : selected_coins) {
            txNew.vin.push_back(CTxIn(coin.outpoint, CScript(), nSequence));
        }

        if (true) {
            int nIn = 0;
            for (const auto& coin : selected_coins) {
                const CScript& scriptPubKey = coin.txout.scriptPubKey;
                SignatureData sigdata;

                if (!ProduceSignature(*signingProvider,
                        MutableTransactionSignatureCreator(
                            &txNew, nIn, coin.txout.nValue, SIGHASH_ALL),
                        scriptPubKey, sigdata)) {
                    strFailReason = "Signing transaction failed";
                    return false;
                } else {
                    UpdateInput(txNew.vin.at(nIn), sigdata);
                }

                nIn++;
            }
        }

        // Return the constructed transaction data.
        tx = MakeTransactionRef(std::move(txNew));

        // Limit size
        if (GetTransactionWeight(*tx) > MAX_STANDARD_TX_WEIGHT) {
            strFailReason = "Transaction too large";
            return false;
        }
    }

    return true;
}

DBErrors CWallet::LoadWallet(bool& fFirstRunRet)
{
    fFirstRunRet = false;
    DBErrors nLoadWalletRet = WalletBatch(*database, "cr+").LoadWallet(this);
    if (nLoadWalletRet == DB_NEED_REWRITE) {
        if (database->Rewrite("\x04pool")) {
            LOCK(cs_wallet);
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    //    {
    //        LOCK2(cs_main, cs_wallet);
    //        for (auto& pair : mapWallet) {
    //            for(unsigned int i = 0; i < pair.second.tx->vout.size(); ++i) {
    //                if (IsMine(pair.second.tx->vout[i]) && !IsSpent(pair.first, i)) {
    //                    setWalletUTXO.insert(COutPoint(pair.first, i));
    //                }
    //            }
    //        }
    //    }

    if (nLoadWalletRet != DB_LOAD_OK)
        return nLoadWalletRet;
    fFirstRunRet = !vchDefaultKey.IsValid();

    // TODO(yuraolex): testing code for is our address
    //    auto scriptPubKey = ParseHex("00146d09da8631adc5fd147757a1b35fafc0ae85fa37");

    //    CTxDestination dest;
    //    ExtractDestination(CScript(scriptPubKey.begin(), scriptPubKey.end()), dest);
    //    auto keyID = boost::apply_visitor(CTxDestinationToKeyIDVisitor(), dest);

    //    auto testChain = [keyID](const CHDChain &chain) {
    //        auto derviveKey = [&](int account, bool fInternal, int fIndex) {
    //            CExtKey result;
    //            chain.DeriveChildExtKey(384, account, fInternal, fIndex, result);
    //            return result.Neuter().pubkey;
    //        };
    //        for(size_t acc = 0; acc < 10; ++acc)
    //        {
    //            for(size_t i = 0; i < 200; ++i)
    //            {
    //                auto external = derviveKey(acc, 0, i).GetID();
    //                auto internal = derviveKey(acc, 1, i).GetID();

    //                if(external == keyID || internal == keyID)
    //                {
    //                    std::cout << "Found match at index:" << i << std::endl;
    //                }
    //            }
    //        }
    //    };

    //    testChain(hdChain);
    //    testChain(auxHdChain);

    DeriveIdentityPubKey();

    return DB_LOAD_OK;
}

void CWallet::InitHDChainAccounts(std::vector<AssetID> assets)
{
    bitcoin::CHDChain hdChain;
    if (GetHDChain(hdChain)) {
        auto scope = hdChain.GetChainScope(HD_PURPOSE_WALLET_KEYS);

        auto pred = [&scope](const auto& id) { return scope.CountAccounts(id) == 0; };

        std::vector<AssetID> filteredAssets;
        boost::copy(assets | filtered(pred), std::back_inserter(filteredAssets));

        if (!filteredAssets.empty()) {
            boost::for_each(filteredAssets, [&scope](const auto& id) { scope.AddAccount(id); });

            WalletBatch batch(*database);
            hdChain.SetChainScope(HD_PURPOSE_WALLET_KEYS, scope);
            SaveCryptedHDChain(batch, hdChain);
        }
    } else {
        throw std::runtime_error(std::string(__func__) + ": HDChain is null!");
    }
}

#if 0
DBErrors CWallet::ZapWalletTx(std::vector<CWalletTx>& vWtx)
{
    DBErrors nZapWalletTxRet = CWalletDB(strWalletFile,"cr+").ZapWalletTx(this, vWtx);
    if (nZapWalletTxRet == DB_NEED_REWRITE)
    {
        if (CDB::Rewrite(strWalletFile, "\x04pool"))
        {
            LOCK(cs_wallet);
            setInternalKeyPool.clear();
            setExternalKeyPool.clear();
            // Note: can't top-up keypool here, because wallet is locked.
            // User will be prompted to unlock wallet the next operation
            // that requires a new key.
        }
    }

    if (nZapWalletTxRet != DB_LOAD_OK)
        return nZapWalletTxRet;

    return DB_LOAD_OK;
}
#endif

bool CWallet::SetAddressBook(WalletBatch& batch, const std::string& address, AssetID assetID,
    const string& strName, const string& strPurpose)
{
    bool fUpdated = false;
    {
        LOCK(cs_wallet); // mapAddressBook
        auto mi = mapAddressBook[assetID].find(address);
        fUpdated = mi != mapAddressBook[assetID].end();
        mapAddressBook[assetID][address].name = strName;
        if (!strPurpose.empty()) /* update purpose only if requested */
            mapAddressBook[assetID][address].purpose = strPurpose;
    }

    if (!strPurpose.empty() && !batch.WritePurpose(assetID, address, strPurpose))
        return false;

    return batch.WriteName(assetID, address, strName);
}

bool CWallet::DelAddressBook(const string& address)
{
#if 0
    {
        LOCK(cs_wallet); // mapAddressBook

        // Delete destdata tuples associated with address
        std::string strAddress = CBitcoinAddress(address).ToString();
        for (auto &&item : mapAddressBook[address].destdata) {
            CWalletDB(strWalletFile, strDatabaseEnvPath).EraseDestData(strAddress, item.first);
        }
        mapAddressBook.erase(address);
    }

    NotifyAddressBookChanged(this, address, "", ::IsMine(*this, address) != ISMINE_NO, "", CT_DELETED);

    if (!fFileBacked)
        return false;
    CWalletDB(strWalletFile, strDatabaseEnvPath).ErasePurpose(CBitcoinAddress(address).ToString());
    return CWalletDB(strWalletFile, strDatabaseEnvPath).EraseName(CBitcoinAddress(address).ToString());
#endif
    return false;
}

#if 0
static void LoadReserveKeysToSet(std::set<CKeyID>& setAddress, const std::set<int64_t>& setKeyPool, CWalletDB& walletdb)
{
    for(const int64_t& id : setKeyPool)
    {
        CKeyPool keypool;
        if (!walletdb.ReadPool(id, keypool))
            throw std::runtime_error(std::string(__func__) + ": read failed");
        assert(keypool.vchPubKey.IsValid());
        CKeyID keyID = keypool.vchPubKey.GetID();
        setAddress.insert(keyID);
    }
}

void CWallet::GetAllReserveKeys(set<CKeyID>& setAddress) const
{
    setAddress.clear();

    CWalletDB walletdb(strWalletFile);

    LOCK2(cs_main, cs_wallet);
    LoadReserveKeysToSet(setAddress, setInternalKeyPool, walletdb);
    LoadReserveKeysToSet(setAddress, setExternalKeyPool, walletdb);

    for (const CKeyID& keyID : setAddress) {
        if (!HaveKey(keyID)) {
            throw std::runtime_error(std::string(__func__) + ": unknown key in key pool");
        }
    }
}
#endif

#if 0

unsigned int CWallet::GetKeyPoolSize() const
{
    AssertLockHeld(cs_wallet); // set{Ex,In}ternalKeyPool
    return setInternalKeyPool.size() + setExternalKeyPool.size();
}
#endif

/** @} */ // end of Actions

/** A class to identify which pubkeys a script and a keystore have in common. */
class CAffectedKeysVisitor : public boost::static_visitor<void> {
private:
    const CKeyStore& keystore;
    std::vector<CKeyID>& vKeys;
    uint32_t nCoinType;

public:
    /**
     * @param[in] keystoreIn The CKeyStore that is queried for the presence of a pubkey.
     * @param[out] vKeysIn A vector to which a script's pubkey identifiers are appended if they are
     * in the keystore.
     */
    CAffectedKeysVisitor(
        uint32_t coinType, const CKeyStore& keystoreIn, std::vector<CKeyID>& vKeysIn)
        : keystore(keystoreIn)
        , vKeys(vKeysIn)
        , nCoinType(coinType)
    {
    }

    /**
     * Apply the visitor to each destination in a script, recursively to the redeemscript
     * in the case of p2sh destinations.
     * @param[in] script The CScript from which destinations are extracted.
     * @post Any CKeyIDs that script and keystore have in common are appended to the visitor's
     * vKeys.
     */
    void Process(const CScript& script)
    {
        txnouttype type;
        std::vector<CTxDestination> vDest;
        int nRequired;
        if (ExtractDestinations(script, type, vDest, nRequired)) {
            for (const CTxDestination& dest : vDest)
                boost::apply_visitor(*this, dest);
        }
    }

    void operator()(const CKeyID& keyId)
    {
        if (keystore.HaveKey(nCoinType, keyId))
            vKeys.push_back(keyId);
    }

    void operator()(const CScriptID& scriptId)
    {
        CScript script;
        //        if (keystore.GetCScript(nCoinType, scriptId, script))
        //            Process(script);
    }

    void operator()(const WitnessV0ScriptHash& scriptID)
    {
        CScriptID id;
        CRIPEMD160().Write(scriptID.begin(), 32).Finalize(id.begin());
        CScript script;
        //        if (keystore.GetCScript(nCoinType, id, script)) {
        //            Process(script);
        //        }
    }

    void operator()(const WitnessV0KeyHash& keyid)
    {
        CKeyID id(keyid);
        if (keystore.HaveKey(nCoinType, id)) {
            vKeys.push_back(id);
        }
    }

    template <typename X> void operator()(const X& none) {}
};

#if 0

bool CWallet::AddDestData(const CTxDestination& dest, const std::string& key, const std::string& value)
{
    if (boost::get<CNoDestination>(&dest))
        return false;

    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile, strDatabaseEnvPath).WriteDestData(CBitcoinAddress(dest).ToString(), key, value);
}

bool CWallet::EraseDestData(const CTxDestination& dest, const std::string& key)
{
    if (!mapAddressBook[dest].destdata.erase(key))
        return false;
    if (!fFileBacked)
        return true;
    return CWalletDB(strWalletFile, strDatabaseEnvPath).EraseDestData(CBitcoinAddress(dest).ToString(), key);
}

bool CWallet::LoadDestData(const CTxDestination& dest, const std::string& key, const std::string& value)
{
    mapAddressBook[dest].destdata.insert(std::make_pair(key, value));
    return true;
}

bool CWallet::GetDestData(const CTxDestination& dest, const std::string& key, std::string* value) const
{
    std::map<CTxDestination, CAddressBookData>::const_iterator i = mapAddressBook.find(dest);
    if (i != mapAddressBook.end()) {
        CAddressBookData::StringMap::const_iterator j = i->second.destdata.find(key);
        if (j != i->second.destdata.end()) {
            if (value)
                *value = j->second;
            return true;
        }
    }
    return false;
}

#endif

void CWallet::GenerateNewHDChain(
    std::string strSeed, std::string strMnemonic, std::string strMnemonicPassphrase)
{
    CHDChain newHdChain;

    newHdChain.GetChainScope(HD_PURPOSE_WALLET_KEYS);
    newHdChain.GetChainScope(HD_PURPUSE_LND);

    if (!strSeed.empty() && IsHex(strSeed)) {
        std::vector<unsigned char> vchSeed = ParseHex(strSeed);
        if (!newHdChain.SetSeed(SecureVector(vchSeed.begin(), vchSeed.end()), true))
            throw std::runtime_error(std::string(__func__) + ": SetSeed failed");

    } else if (!strMnemonic.empty()) {
        //        LogPrintf("CWallet::GenerateNewHDChain -- Incorrect seed, generating random one
        //        instead\n");

        // NOTE: empty mnemonic means "generate a new one for me"
        // NOTE: default mnemonic passphrase is an empty string

        SecureVector vchMnemonic(strMnemonic.begin(), strMnemonic.end());
        SecureVector vchMnemonicPassphrase(
            strMnemonicPassphrase.begin(), strMnemonicPassphrase.end());

        if (!newHdChain.SetMnemonic(vchMnemonic, vchMnemonicPassphrase, true))
            throw std::runtime_error(std::string(__func__) + ": SetMnemonic failed");

    } else {
        assert(false);
    }
    newHdChain.Debug(__func__);

    {
        WalletBatch batch(*database);
        if (!SetHDChain(&batch, newHdChain, false))
            throw std::runtime_error(std::string(__func__) + ": SetHDChain failed");
    }

    DeriveIdentityPubKey();
}

bool CWallet::SetHDChain(WalletBatch* batch, const CHDChain& chain, bool memonly)
{
    assert(memonly ? batch == nullptr : batch != nullptr);

    LOCK(cs_wallet);

    CCryptoKeyStore::SetHDChain(chain);

    if (!memonly && !batch->WriteHDChain(chain))
        throw std::runtime_error(std::string(__func__) + ": WriteHDChain failed");

    return true;
}

bool CWallet::SetCryptedHDChain(WalletBatch* batch, const CHDChain& chain, bool memonly)
{
    assert(memonly ? batch == nullptr : batch != nullptr);

    LOCK(cs_wallet);

    if (!CCryptoKeyStore::SetCryptedHDChain(chain))
        return false;

    if (!memonly) {
        if (encrypted_batch) {
            if (!encrypted_batch->WriteCryptedHDChain(chain))
                throw std::runtime_error(std::string(__func__) + ": WriteCryptedHDChain failed");
        } else {
            if (!batch->WriteCryptedHDChain(chain))
                throw std::runtime_error(std::string(__func__) + ": WriteCryptedHDChain failed");
        }
    }

    return true;
}

bool CWallet::GetDecryptedHDChain(CHDChain& hdChainRet) const
{
    LOCK(cs_wallet);

    CHDChain hdChainTmp;
    if (!GetHDChain(hdChainTmp)) {
        return false;
    }

    if (!DecryptHDChain(hdChainTmp))
        return false;

    // make sure seed matches this chain
    if (hdChainTmp.GetID() != hdChainTmp.GetSeedHash())
        return false;

    hdChainRet = hdChainTmp;

    return true;
}
}
