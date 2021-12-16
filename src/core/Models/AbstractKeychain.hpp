#ifndef ABSTRACTKEYCHAIN_HPP
#define ABSTRACTKEYCHAIN_HPP

#include <Tools/Common.hpp>
#include <Utils/Utils.hpp>
#include <boost/optional.hpp>
#include <cstdint>
#include <string>
#include <tuple>

/*
 * This file has abstract keychain that will be used from lnd for different
 * lnd related purposes.
 */

// Family, index: m/201'/coinType'/keyFamily/0/index
using KeyLocator = std::pair<uint32_t, uint32_t>;
// KeyLocator +? HexEncoded PubKey(might be empty)
using KeyDescriptor = std::tuple<KeyLocator, std::string>;

class AbstractKeychain {
public:
    AbstractKeychain();
    virtual ~AbstractKeychain();

    // DeriveNextKey attempts to derive the *next* key within the key
    // family (account in BIP43) specified. This method should return the
    // next external child within this branch.
    virtual Promise<KeyDescriptor> deriveNextKey(AssetID assetID, uint32_t keyFamily) = 0;

    // DeriveKey attempts to derive an arbitrary key specified by the
    // passed KeyLocator. This may be used in several recovery scenarios,
    // or when manually rotating something like our current default node
    // key.
    virtual Promise<KeyDescriptor> deriveKey(AssetID assetID, KeyLocator locator) = 0;

    // Derives priv key, returns hex encoded priv key
    virtual Promise<std::string> derivePrivKey(AssetID assetID, KeyDescriptor descriptor) = 0;
};

#endif // ABSTRACTKEYCHAIN_HPP
