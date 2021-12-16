#include "Encodings.hpp"
#include <QCryptographicHash>
#include <QDebug>
#include <eth_utils/eth_utils.h>
#include <key.h>
#include <utilstrencodings.h>

namespace eth {

//==============================================================================

QString erc20::balanceOfPayload(QString address)
{
    QCryptographicHash keccak(QCryptographicHash::Keccak_256);
    keccak.addData("balanceOf(address)");
    QString result = keccak.result().left(4).toHex();
    address = address.mid(2).toLower();
    Q_ASSERT_X(address.size() == 40, __FUNCTION__, "Invalid eth address size");
    result.append(QString('0').repeated(24));
    result.append(address);
    return "0x" + result;
}

//==============================================================================

QString erc20::transferPayload(QString address, u256 value)
{
    QCryptographicHash keccak(QCryptographicHash::Keccak_256);
    keccak.addData("transfer(address,uint256)");
    QString result = keccak.result().left(4).toHex();
    address = address.mid(2).toLower();
    Q_ASSERT_X(address.size() == 40, __FUNCTION__, "Invalid eth address size");
    result.append(QString('0').repeated(24));
    result.append(address);

    std::stringstream ss;
    ss << std::setfill('0') << std::setw(64) << std::hex << value;
    result.append(QString::fromStdString(ss.str()));
    return "0x" + result;
}

//==============================================================================

SignedTransaction SignTransaction(Tx tx, uint32_t nChainId, const bitcoin::CKey& secret)
{
    std::vector<const char*> data;

    tx.normalize();

    std::string empty{ "" };
    std::string hexChainId = std::string{ "0" } + std::to_string(nChainId);

    data.push_back(tx.nonce.data());
    data.push_back(tx.gasPrice.data());
    data.push_back(tx.gasLimit.data());
    data.push_back(tx.to.data());
    data.push_back(tx.value.data());
    data.push_back(tx.data.data());
    data.push_back(hexChainId.data());
    data.push_back(empty.data());
    data.push_back(empty.data());

    auto result = rust_eth_rlp(&data[0], data.size());
    auto raw = QByteArray::fromHex(QByteArray::fromStdString(result));
    rust_eth_free_data(result);

    SignedTransaction signedTx{ tx };
    QCryptographicHash sha3(QCryptographicHash::Keccak_256);
    sha3.addData(raw);

    std::vector<unsigned char> vchSig;
    auto signingHash = sha3.result();
    secret.SignCompact(
        bitcoin::uint256{ std::vector<unsigned char>(signingHash.begin(), signingHash.end()) },
        vchSig);

    signedTx.sig.r = bitcoin::HexStr(vchSig.begin() + 1, vchSig.begin() + 33);
    signedTx.sig.s = bitcoin::HexStr(vchSig.begin() + 33, vchSig.end());
    // bitcoin implementation requires 27 + {0, 1} + 4(for comporessed sigs) as vchSig[0]
    // eth wants only 27 + {0, 1} or 35 + chain_id * 2 + {0,1} depending on usage of EIP155,
    // we will use EIP155, so let's just tweak it
    signedTx.sig.v = static_cast<uint32_t>(vchSig[0]) - (27 + 4) + nChainId * 2 + 35;

    return signedTx;
}

//==============================================================================

QString ConvertChainId(uint32_t chainId)
{
    static std::unordered_map<uint32_t, QString> mapping{ { 1, "mainnet" }, { 3, "ropsten" },
        { 4, "rinkeby" }, { 5, "goerli" } };

    Q_ASSERT_X(mapping.count(chainId) > 0, __FUNCTION__,
        QString("Unsupported chainId: %1").arg(chainId).toStdString().data());
    return mapping.at(chainId);
}

//==============================================================================

std::string SignedTransaction::toHex() const
{
    std::vector<const char*> values;

    std::string empty{ "" };

    std::string v = QString::number(sig.v, 16).toStdString();

    values.push_back(nonce.data());
    values.push_back(gasPrice.data());
    values.push_back(gasLimit.data());
    values.push_back(to.data());
    values.push_back(value.data());
    values.push_back(data.data());
    values.push_back(v.data());
    values.push_back(sig.r.data());
    values.push_back(sig.s.data());

    auto rustResult = rust_eth_rlp(&values[0], values.size());
    std::string r{ rustResult };
    rust_eth_free_data(rustResult);
    return "0x" + r;
}

//==============================================================================

void Tx::normalize()
{
    auto normalizeHex = [](auto& str) {
        if (str.substr(0, 2) == "0x") {
            str = str.substr(2);
        }
        if (str.length() % 2 != 0) {
            str = '0' + str;
        }
    };

    normalizeHex(nonce);
    if (nonce == "00") {
        nonce.clear();
    }
    normalizeHex(gasPrice);
    normalizeHex(gasLimit);
    normalizeHex(to);
    normalizeHex(value);
    normalizeHex(data);
}

//==============================================================================

u256 ConvertFromHex(const QString& hex)
{
    return ConvertFromHex(hex.toStdString());
}

//==============================================================================

u256 ConvertFromHex(const std::string& hex)
{
    // u256 accepts only strings in format 0x, otherwise it can't know what is radix
    // of number
    auto value = hex.substr(0, 2) == "0x" ? hex : ("0x" + hex);
    return u256{ value };
}

//==============================================================================

QByteArray FromHex(QString hash)
{
    auto value = hash.left(2) == "0x" ? hash.mid(2) : hash;
    return QByteArray::fromHex(value.toLatin1());
}

//==============================================================================

std::string ExportBitsData(eth::u256 value)
{
    std::vector<unsigned char> v;
    boost::multiprecision::export_bits(value, std::back_inserter(v), 8);
    return std::string(v.begin(), v.end());
}

//==============================================================================

std::string ConvertToHex(u256 value)
{
    std::stringstream ss;
    ss << std::hex << std::showbase << value;
    return ss.str();
}

//==============================================================================

QString ChecksumAddress(QByteArray bytes)
{
    return ChecksumAddress(QString("0x%1").arg(QString::fromLatin1(bytes.toHex())));
}

//==============================================================================

QString ChecksumAddress(QString address)
{
    address = address.replace("0x", "").toLower();
    QCryptographicHash keccak(QCryptographicHash::Keccak_256);
    keccak.addData(address.toLatin1());
    auto hash = keccak.result().toHex();
    QString result = QString("0x%1").arg(address);
    // based on this github issue: https://github.com/ethereum/EIPs/issues/55#issuecomment-187159063
    for (int i = 0; i < address.size(); ++i) {
        result[i + 2]
            = !address[i].isDigit() && (static_cast<uint8_t>(hash[i].operator char()) >= 56)
            ? address[i].toUpper()
            : address[i];
    }

    return result;
}

//==============================================================================

bool CompareAddress(const QString &left, const QString &right)
{
    return left.toLower() == right.toLower();
}

//==============================================================================
}
