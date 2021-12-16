// Copyright (c) 2014-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <key_io.h>

#include <base58.h>
#include <bech32.h>
#include <chainparams.hpp>
#include <script/script.h>
#include <tinyformat.h>
#include <utilstrencodings.h>

#include <boost/variant/apply_visitor.hpp>
#include <boost/variant/static_visitor.hpp>

#include <algorithm>
#include <assert.h>
#include <string.h>

#include <QCryptographicHash>

namespace bitcoin {

class DestinationEncoder : public boost::static_visitor<std::string> {
private:
    const CChainParams& m_params;

public:
    explicit DestinationEncoder(const CChainParams& params)
        : m_params(params)
    {
    }

    std::string operator()(const CKeyID& id) const
    {
        std::vector<unsigned char> data
            = m_params.base58Prefix(CChainParams::Base58Type::PUBKEY_ADDRESS);
        data.insert(data.end(), id.begin(), id.end());
        return EncodeBase58Check(data);
    }

    std::string operator()(const CScriptID& id) const
    {
        std::vector<unsigned char> data
            = m_params.base58Prefix(CChainParams::Base58Type::SCRIPT_ADDRESS);
        data.insert(data.end(), id.begin(), id.end());
        return EncodeBase58Check(data);
    }

    std::string operator()(const WitnessV0KeyHash& id) const
    {
        std::vector<unsigned char> data = { 0 };
        data.reserve(33);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.begin(), id.end());
        return bech32::Encode(m_params.bech32HRP(), data);
    }

    std::string operator()(const WitnessV0ScriptHash& id) const
    {
        std::vector<unsigned char> data = { 0 };
        data.reserve(53);
        ConvertBits<8, 5, true>([&](unsigned char c) { data.push_back(c); }, id.begin(), id.end());
        return bech32::Encode(m_params.bech32HRP(), data);
    }

    std::string operator()(const WitnessUnknown& id) const
    {
        if (id.version < 1 || id.version > 16 || id.length < 2 || id.length > 40) {
            return {};
        }
        std::vector<unsigned char> data = { (unsigned char)id.version };
        data.reserve(1 + (id.length * 8 + 4) / 5);
        ConvertBits<8, 5, true>(
            [&](unsigned char c) { data.push_back(c); }, id.program, id.program + id.length);
        return bech32::Encode(m_params.bech32HRP(), data);
    }

    std::string operator()(const CNoDestination& no) const { return {}; }
};

CTxDestination DecodeDestination(const std::string& str, const CChainParams& params)
{
    std::vector<unsigned char> data;
    uint160 hash;
    if (DecodeBase58Check(str, data)) {
        // base58-encoded Bitcoin addresses.
        // Public-key-hash-addresses have version 0 (or 111 testnet).
        // The data vector contains RIPEMD160(SHA256(pubkey)), where pubkey is the serialized public
        // key.
        const std::vector<unsigned char>& pubkey_prefix
            = params.base58Prefix(CChainParams::Base58Type::PUBKEY_ADDRESS);
        if (data.size() == hash.size() + pubkey_prefix.size()
            && std::equal(pubkey_prefix.begin(), pubkey_prefix.end(), data.begin())) {
            std::copy(data.begin() + pubkey_prefix.size(), data.end(), hash.begin());
            return CKeyID(hash);
        }
        // Script-hash-addresses have version 5 (or 196 testnet).
        // The data vector contains RIPEMD160(SHA256(cscript)), where cscript is the serialized
        // redemption script.
        const std::vector<unsigned char>& script_prefix
            = params.base58Prefix(CChainParams::Base58Type::SCRIPT_ADDRESS);
        if (data.size() == hash.size() + script_prefix.size()
            && std::equal(script_prefix.begin(), script_prefix.end(), data.begin())) {
            std::copy(data.begin() + script_prefix.size(), data.end(), hash.begin());
            return CScriptID(hash);
        }
    }
    data.clear();
    auto bech = bech32::Decode(str);
    if (bech.second.size() > 0 && bech.first == params.bech32HRP()) {
        // Bech32 decoding
        int version = bech.second[0]; // The first 5 bit symbol is the witness version (0-16)
        // The rest of the symbols are converted witness program bytes.
        data.reserve(((bech.second.size() - 1) * 5) / 8);
        if (ConvertBits<5, 8, false>([&](unsigned char c) { data.push_back(c); },
                bech.second.begin() + 1, bech.second.end())) {
            if (version == 0) {
                {
                    WitnessV0KeyHash keyid;
                    if (data.size() == keyid.size()) {
                        std::copy(data.begin(), data.end(), keyid.begin());
                        return keyid;
                    }
                }
                {
                    WitnessV0ScriptHash scriptid;
                    if (data.size() == scriptid.size()) {
                        std::copy(data.begin(), data.end(), scriptid.begin());
                        return scriptid;
                    }
                }
                return CNoDestination();
            }
            if (version > 16 || data.size() < 2 || data.size() > 40) {
                return CNoDestination();
            }
            WitnessUnknown unk;
            unk.version = version;
            std::copy(data.begin(), data.end(), unk.program);
            unk.length = data.size();
            return unk;
        }
    }
    return CNoDestination();
}

CKey DecodeSecret(const std::string& str, const CChainParams& params)
{
    CKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data)) {
        const std::vector<unsigned char>& privkey_prefix
            = params.base58Prefix(CChainParams::Base58Type::SECRET_KEY);
        if ((data.size() == 32 + privkey_prefix.size()
                || (data.size() == 33 + privkey_prefix.size() && data.back() == 1))
            && std::equal(privkey_prefix.begin(), privkey_prefix.end(), data.begin())) {
            bool compressed = data.size() == 33 + privkey_prefix.size();
            key.Set(data.begin() + privkey_prefix.size(), data.begin() + privkey_prefix.size() + 32,
                compressed);
        }
    }
    memory_cleanse(data.data(), data.size());
    return key;
}

std::string EncodeSecret(const CKey& key, const CChainParams& params)
{
    assert(key.IsValid());
    std::vector<unsigned char> data = params.base58Prefix(CChainParams::Base58Type::SECRET_KEY);
    data.insert(data.end(), key.begin(), key.end());
    if (key.IsCompressed()) {
        data.push_back(1);
    }
    std::string ret = EncodeBase58Check(data);
    memory_cleanse(data.data(), data.size());
    return ret;
}

CExtPubKey DecodeExtPubKey(const std::string& str, const CChainParams& params)
{
    CExtPubKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data)) {
        const std::vector<unsigned char>& prefix
            = params.base58Prefix(CChainParams::Base58Type::EXT_PUBLIC_KEY);
        if (data.size() == BIP32_EXTKEY_SIZE + prefix.size()
            && std::equal(prefix.begin(), prefix.end(), data.begin())) {
            key.Decode(data.data() + prefix.size());
        }
    }
    return key;
}

std::string EncodeExtPubKey(const CExtPubKey& key, const CChainParams& params)
{
    std::vector<unsigned char> data = params.base58Prefix(CChainParams::Base58Type::EXT_PUBLIC_KEY);
    size_t size = data.size();
    data.resize(size + BIP32_EXTKEY_SIZE);
    key.Encode(data.data() + size);
    std::string ret = EncodeBase58Check(data);
    return ret;
}

CExtKey DecodeExtKey(const std::string& str, const CChainParams& params)
{
    CExtKey key;
    std::vector<unsigned char> data;
    if (DecodeBase58Check(str, data)) {
        const std::vector<unsigned char>& prefix
            = params.base58Prefix(CChainParams::Base58Type::EXT_SECRET_KEY);
        if (data.size() == BIP32_EXTKEY_SIZE + prefix.size()
            && std::equal(prefix.begin(), prefix.end(), data.begin())) {
            key.Decode(data.data() + prefix.size());
        }
    }
    return key;
}

std::string EncodeExtKey(const CExtKey& key, const CChainParams& params)
{
    std::vector<unsigned char> data = params.base58Prefix(CChainParams::Base58Type::EXT_SECRET_KEY);
    size_t size = data.size();
    data.resize(size + BIP32_EXTKEY_SIZE);
    key.Encode(data.data() + size);
    std::string ret = EncodeBase58Check(data);
    memory_cleanse(data.data(), data.size());
    return ret;
}

std::string EncodeDestination(const CTxDestination& dest, const CChainParams& params)
{
    return boost::apply_visitor(DestinationEncoder(params), dest);
}

bool IsValidDestinationString(const std::string& str, const CChainParams& params)
{
    return IsValidDestination(DecodeDestination(str, params));
}

/**
 * Create the assembly string representation of a CScript object.
 * @param[in] script    CScript object to convert into the asm string representation.
 * @param[in] fAttemptSighashDecode    Whether to attempt to decode sighash types on data within the
 * script that matches the format of a signature. Only pass true for scripts you believe could
 * contain signatures. For example, pass false, or omit the this argument (defaults to false), for
 * scriptPubKeys.
 */
std::string ScriptToAsmStr(const CScript& script)
{
    std::string str;
    opcodetype opcode;
    std::vector<unsigned char> vch;
    CScript::const_iterator pc = script.begin();
    while (pc < script.end()) {
        if (!str.empty()) {
            str += " ";
        }
        if (!script.GetOp(pc, opcode, vch)) {
            str += "[error]";
            return str;
        }
        if (0 <= opcode && opcode <= OP_PUSHDATA4) {
            if (vch.size() <= static_cast<std::vector<unsigned char>::size_type>(4)) {
                str += strprintf("%d", CScriptNum(vch, false).getint());
            } else {
                // the IsUnspendable check makes sure not to try to decode OP_RETURN data that may
                // match the format of a signature
                str += HexStr(vch);
            }
        } else {
            str += GetOpName(opcode);
        }
    }
    return str;
}
}

namespace eth {

std::string EncodeDestination(bitcoin::CPubKey pubkey)
{
    if (pubkey.IsCompressed()) {
        pubkey.Decompress();
    }

    QCryptographicHash keccak(QCryptographicHash::Keccak_256);
    keccak.addData(reinterpret_cast<const char*>(&pubkey[1]), 64);
    auto address = keccak.result().right(20).toHex();
    keccak.reset();
    keccak.addData(address);
    auto addressHash = keccak.result().toHex();
    std::string result(address.size() + 2, '0');
    result[1] = 'x';
    // based on this github issue: https://github.com/ethereum/EIPs/issues/55#issuecomment-187159063
    for (int i = 0; i < address.size(); ++i) {
        result[i + 2] = !isdigit(address[i]) && (static_cast<uint8_t>(addressHash[i]) >= 56)
            ? (static_cast<uint8_t>(address[i]) - 32)
            : address[i];
    }

    return result;
}
}
