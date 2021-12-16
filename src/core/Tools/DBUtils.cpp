#include "DBUtils.hpp"
//#include <QString>

namespace DBUtils {

//==============================================================================

bitcoin::CDiskBlockIndex BlockIndexToDisk(const Wire::VerboseBlockHeader& index)
{
    const Wire::VerboseBlockHeader::Header& header = index.header;
    const auto& filter = index.filter;
    return bitcoin::CDiskBlockIndex(header.version, bitcoin::uint256S(header.prevBlock),
        bitcoin::uint256S(header.merkleRoot), header.timestamp, header.bits, header.nonce,
        index.height, bitcoin::uint256S(index.hash),
        bitcoin::CDiskBlockFilter(filter.n, filter.m, filter.p, filter.bytes));
}

//==============================================================================

Wire::VerboseBlockHeader DiskIndexToBlockIndex(const bitcoin::CDiskBlockIndex& diskIndex)
{
    Wire::VerboseBlockHeader header;
    header.header.version = diskIndex.nVersion;
    header.header.prevBlock = diskIndex.hashPrevBlock.ToString();
    header.header.merkleRoot = diskIndex.hashMerkleRoot.ToString();
    header.header.bits = diskIndex.nBits;
    header.header.nonce = diskIndex.nNonce;
    header.header.timestamp = diskIndex.nTimestamp;

    header.height = diskIndex.nHeight;
    header.hash = diskIndex.hashBlock.ToString();

    const auto& filter = diskIndex.blockFilter;
    header.filter = Wire::EncodedBlockFilter(filter.n, filter.m, filter.p, filter.vchBlockFilter);

    return header;
}

//==============================================================================
}
