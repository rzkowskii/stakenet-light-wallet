#ifndef BLOCKHEADER_HPP
#define BLOCKHEADER_HPP

#include <Data/TransactionEntry.hpp>
#include <QString>
#include <limits>
#include <uint256.h>
#include <vector>

class QJsonObject;

namespace Wire {

struct EncodedBlockFilter {
    EncodedBlockFilter() = default;
    explicit EncodedBlockFilter(uint32_t nIn, uint64_t mIn, uint16_t pIn, QString hexEncodedFilter);
    explicit EncodedBlockFilter(
        uint32_t nIn, uint64_t mIn, uint16_t pIn, std::vector<uint8_t> encodedFilter);

    bool isValid() const;

    static EncodedBlockFilter FromJson(const QJsonObject& object);

    uint32_t n;
    uint64_t m;
    uint16_t p;
    std::vector<uint8_t> bytes;
};

struct VerboseBlockHeader {

    static VerboseBlockHeader FromJson(const QJsonObject& obj);

    struct Header {
        int32_t version;
        std::string prevBlock;
        std::string merkleRoot;
        uint32_t timestamp;
        uint32_t bits;
        uint32_t nonce;
    } header;

    uint32_t height;
    std::string hash;
    EncodedBlockFilter filter;
};

struct OutPoint {
    bitcoin::uint256 hash;
    uint32_t index = (std::numeric_limits<uint32_t>::max)();

    friend bool operator<(const OutPoint& a, const OutPoint& b)
    {
        int cmp = a.hash.Compare(b.hash);
        return cmp < 0 || (cmp == 0 && a.index < b.index);
    }
};

struct TxIn {
    OutPoint previousOutPoint;
    std::vector<unsigned char> signatureScript;
    std::vector<std::vector<unsigned char>> witness;
    uint32_t sequence{ 0xffffffff };
};

struct TxOut {
    int64_t value;
    std::vector<unsigned char> pkScript;

    static TxOut FromJson(const QJsonObject& obj);
};

struct MsgTx {
    int32_t version;
    std::vector<TxIn> txIn;
    std::vector<TxOut> txOut;
    uint32_t lockTime{ 0 };
};

struct TxConfrimation {
    std::string blockHash;
    uint32_t blockHeight { 0 };
    uint32_t txIndex { 0 };
    std::string hexTx;

    static TxConfrimation FromJson(const QJsonObject& obj);
};

struct MsgBlock {
    VerboseBlockHeader::Header header;
    std::vector<MsgTx> transactions;
};

struct MsgEncodedBlock {
    VerboseBlockHeader::Header header;
    std::vector<std::string> transactions;
};

struct StrippedBlock {
    StrippedBlock() = default;
    explicit StrippedBlock(QString hashIn)
        : hash(hashIn)
    {
    }

    void addTransactions(OnChainTxList data);

    QString hash;
    OnChainTxList transactions;
};
}

#endif // BLOCKHEADER_HPP
