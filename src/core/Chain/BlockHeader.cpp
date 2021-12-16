#include "BlockHeader.hpp"

#include <QJsonObject>
#include <utilstrencodings.h>

//==============================================================================

void Wire::StrippedBlock::addTransactions(OnChainTxList data)
{
    std::copy(std::begin(data), std::end(data), std::back_inserter(transactions));
}

//==============================================================================

Wire::EncodedBlockFilter::EncodedBlockFilter(
    uint32_t nIn, uint64_t mIn, uint16_t pIn, QString hexEncodedFilter)
    : EncodedBlockFilter(nIn, mIn, pIn, bitcoin::ParseHex(hexEncodedFilter.toStdString()))
{
}

//==============================================================================

Wire::EncodedBlockFilter::EncodedBlockFilter(
    uint32_t nIn, uint64_t mIn, uint16_t pIn, std::vector<uint8_t> encodedFilter)
    : n(nIn)
    , m(mIn)
    , p(pIn)
    , bytes(encodedFilter)
{
}

//==============================================================================

bool Wire::EncodedBlockFilter::isValid() const
{
    return !bytes.empty();
}

//==============================================================================

Wire::EncodedBlockFilter Wire::EncodedBlockFilter::FromJson(const QJsonObject& object)
{
    return EncodedBlockFilter(object.value("n").toInt(), object.value("m").toInt(),
        object.value("p").toInt(), object.value("hex").toString());
}

//==============================================================================

Wire::VerboseBlockHeader Wire::VerboseBlockHeader::FromJson(const QJsonObject& obj)
{
    Wire::VerboseBlockHeader result;
    result.header.version = obj.value("version").toInt();
    result.header.merkleRoot = obj.value("merkleRoot").toString().toStdString();
    result.header.prevBlock = obj.value("previousBlockhash").toString().toStdString();
    result.header.nonce = static_cast<uint32_t>(obj.value("nonce").toDouble());
    result.header.timestamp = static_cast<uint32_t>(obj.value("time").toDouble());
    result.header.bits = obj.value("bits").toString().toUInt(nullptr, 16);
    result.height = obj.value("height").toInt();
    result.hash = obj.value("hash").toString().toStdString();
    result.filter = EncodedBlockFilter::FromJson(obj.value("filter").toObject());
    return result;
}

//==============================================================================

Wire::TxConfrimation Wire::TxConfrimation::FromJson(const QJsonObject& obj)
{
    TxConfrimation result;
    result.blockHash = obj.value("blockhash").toString().toStdString();
    result.hexTx = obj.value("hex").toString().toStdString();
    result.txIndex = obj.value("index").toInt();
    result.blockHeight = obj.value("height").toInt();
    return result;
}

//==============================================================================

Wire::TxOut Wire::TxOut::FromJson(const QJsonObject& obj)
{
    Wire::TxOut result;
    result.value = static_cast<int64_t>(obj.value("value").toString().toLongLong());
    result.pkScript = bitcoin::ParseHex(obj.value("script").toString().toStdString());
    return result;
}
