#include "BlockIndex.hpp"
#include <crypto/common.h>
#include <uint256.h>
#include <utilstrencodings.h>

//==============================================================================

BlockIndex::BlockIndex(Wire::VerboseBlockHeader verboseHeader)
    : _header(verboseHeader)
{
}

//==============================================================================

const Wire::VerboseBlockHeader::Header& BlockIndex::header() const
{
    return _header.header;
}

//==============================================================================

const Wire::EncodedBlockFilter& BlockIndex::filter() const
{
    return _header.filter;
}

//==============================================================================

bool BlockIndex::operator<(const BlockIndex& rhs) const
{
    auto compareHelper = [](std::string hash) { return bitcoin::uint256S(hash); };

    return compareHelper(_header.hash) < compareHelper(rhs._header.hash);
}

//==============================================================================

size_t BlockIndex::height() const
{
    return _header.height >= 0 ? _header.height : 0;
}

//==============================================================================

BlockHash BlockIndex::hash() const
{
    return QString::fromStdString(_header.hash);
}

//==============================================================================
