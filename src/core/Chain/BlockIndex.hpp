#ifndef BLOCKINDEX_HPP
#define BLOCKINDEX_HPP

#include <Chain/BlockHeader.hpp>
#include <QString>
#include <memory>

struct BlockIndex;

using BlockIndexUniqueRef = std::unique_ptr<BlockIndex>;
using BlockIndexPtr = BlockIndex*;

struct BlockIndex {
public:
    BlockIndex() = default;
    BlockIndex(const BlockIndex&) = default;
    explicit BlockIndex(Wire::VerboseBlockHeader verboseHeader);

    const Wire::VerboseBlockHeader::Header& header() const;
    const Wire::EncodedBlockFilter& filter() const;

    bool operator<(const BlockIndex& rhs) const;

    size_t height() const;
    BlockHash hash() const;

private:
    Wire::VerboseBlockHeader _header;
};

#endif // BLOCKINDEX_HPP
