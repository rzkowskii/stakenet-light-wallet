#ifndef BLOCKFILTERMATCHER_HPP
#define BLOCKFILTERMATCHER_HPP

#include <Tools/Common.hpp>
#include <memory>

struct BlockIndex;

//==============================================================================

struct BlockFilterMatcher {
public:
    BlockFilterMatcher(AssetID assetID);
    virtual ~BlockFilterMatcher();
    virtual bool match(const BlockIndex& blockIndex) const = 0;

protected:
    AssetID _assetID;
};

//==============================================================================

using BFMatcherUniqueRef = std::unique_ptr<BlockFilterMatcher>;

struct BlockFilterMatchable {
    virtual ~BlockFilterMatchable();
    virtual BFMatcherUniqueRef createMatcher(AssetID assetID) const = 0;
};

//==============================================================================

#endif // BLOCKFILTERMATCHER_HPP
