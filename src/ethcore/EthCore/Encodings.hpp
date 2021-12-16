#ifndef ETHENCODINGS_HPP
#define ETHENCODINGS_HPP

#include <EthCore/Types.hpp>
#include <QString>

namespace bitcoin {
class CKey;
}

namespace eth {

//==============================================================================

struct Tx {
    // hex
    std::string nonce;
    // hex
    std::string gasPrice;
    // hex
    std::string gasLimit;
    // hex
    std::string to;
    // hex
    std::string value;
    // hex
    std::string data;

    void normalize();
};

//==============================================================================

struct Signature {
    // hex
    std::string r;
    uint32_t v{ 0 };
    // hex
    std::string s;
};

//==============================================================================

struct SignedTransaction : Tx {
    explicit SignedTransaction(Tx base)
        : Tx(base)
    {
    }

    SignedTransaction() {}

    // Encodes transaction into RLP format, returns HEX as result with 0x preffix
    std::string toHex() const;
    void normalizedNonce();

    Signature sig;
};

//==============================================================================

namespace erc20 {
    QString balanceOfPayload(QString address);
    QString transferPayload(QString address, eth::u256 value);
}

//==============================================================================

SignedTransaction SignTransaction(Tx tx, uint32_t nChainId, const bitcoin::CKey& secret);
QString ConvertChainId(uint32_t chainId);

//==============================================================================

QByteArray FromHex(QString hash);
std::string ExportBitsData(eth::u256 value);
u256 ConvertFromHex(const QString& hex);
u256 ConvertFromHex(const std::string& hex);
std::string ConvertToHex(eth::u256 value);
QString ChecksumAddress(QByteArray bytes);
QString ChecksumAddress(QString address);
bool CompareAddress(const QString &left, const QString &right);

//==============================================================================
}

#endif // ETHENCODINGS_HPP
