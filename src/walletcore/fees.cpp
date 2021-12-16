// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <fees.h>

//#include <policy/policy.h>
//#include <txmempool.h>
//#include <util/system.h>
//#include <validation.h>
//#include <wallet/coincontrol.h>
//#include <wallet/wallet.h>
#include <policy/feerate.h>
#include <transaction.h>

namespace bitcoin {

static const unsigned int DEFAULT_MIN_RELAY_TX_FEE = 1000;
static const CAmount DEFAULT_TRANSACTION_MAXFEE = 0.1 * COIN;

CFeeRate minRelayTxFee = CFeeRate(DEFAULT_MIN_RELAY_TX_FEE);
CAmount maxTxFee = DEFAULT_TRANSACTION_MAXFEE;

CAmount GetRequiredFee(const CWallet& wallet, unsigned int nTxBytes)
{
    return GetRequiredFeeRate(wallet).GetFee(nTxBytes);
}

CAmount GetMinimumFee(const CWallet& wallet, unsigned int nTxBytes)
{
    CAmount fee_needed = GetMinimumFeeRate(wallet).GetFee(nTxBytes);
    // Always obey the maximum
    if (fee_needed > maxTxFee) {
        fee_needed = maxTxFee;
    }

    return fee_needed;
}

CFeeRate GetRequiredFeeRate(const CWallet& wallet)
{
    return minRelayTxFee;
}

CFeeRate GetMinimumFeeRate(const CWallet& wallet)
{
    return minRelayTxFee;
}
}
