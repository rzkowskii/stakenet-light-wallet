#include "chainparams.hpp"
#include <cassert>
#include <string>

namespace bitcoin {

//==============================================================================

CChainParams::CChainParams(
    CChainParams::Base58TypesMap base58Map, ExtCoinType type, std::string bech32Prefix)
    : _base58Types(base58Map)
    , _type(type)
    , _bech32Prefix(bech32Prefix)
{
    assert(_base58Types.count(Base58Type::PUBKEY_ADDRESS) == 1);
    assert(_base58Types.count(Base58Type::SCRIPT_ADDRESS) == 1);
    assert(_base58Types.count(Base58Type::SECRET_KEY) == 1);
    assert(_base58Types.count(Base58Type::EXT_PUBLIC_KEY) == 1);
    assert(_base58Types.count(Base58Type::EXT_SECRET_KEY) == 1);
}

//==============================================================================

CChainParams::~CChainParams() {}

//==============================================================================

const std::vector<unsigned char>& CChainParams::base58Prefix(CChainParams::Base58Type type) const
{
    auto it = _base58Types.find(type);
    return it->second;
}

//==============================================================================

const std::string& CChainParams::bech32HRP() const
{
    return _bech32Prefix;
}

//==============================================================================

CChainParams::ExtCoinType CChainParams::extCoinType() const
{
    return _type;
}

//==============================================================================

std::vector<CChainParams::Base58Type> CChainParams::base58Types() const
{
    std::vector<CChainParams::Base58Type> result;
    for (auto&& pair : _base58Types) {
        result.push_back(pair.first);
    }
    return result;
}

//==============================================================================
}
