#ifndef INTERFACES_HPP
#define INTERFACES_HPP

#include <coinselection.h>
#include <memory>
#include <optional.h>
#include <pubkey.h>
#include <transaction.h>
#include <uint256.h>

namespace bitcoin {
namespace interfaces {
    //! Interface for giving wallet processes access to blockchain state.
    class Chain {
    public:
        virtual ~Chain() {}

        //! Interface for querying locked chain state, used by legacy code that
        //! assumes state won't change between calls. New code should avoid using
        //! the Lock interface and instead call higher-level Chain methods
        //! that return more information so the chain doesn't need to stay locked
        //! between calls.
        class Lock {
        public:
            virtual ~Lock() {}

            //! Get current chain height, not including genesis block (returns 0 if
            //! chain only contains genesis block, nullopt if chain does not contain
            //! any blocks).
            virtual boost::optional<int> getHeight() = 0;

            //! Get block height above genesis block. Returns 0 for genesis block,
            //! 1 for following block, and so on. Returns nullopt for a block not
            //! included in the current chain.
            virtual boost::optional<int> getBlockHeight(const uint256& hash) = 0;

            //! Get block depth. Returns 1 for chain tip, 2 for preceding block, and
            //! so on. Returns 0 for a block not included in the current chain.
            virtual int getBlockDepth(const uint256& hash) = 0;

            //! Get block hash. Height must be valid or this function will abort.
            virtual uint256 getBlockHash(int height) = 0;

            //! Get block time. Height must be valid or this function will abort.
            virtual int64_t getBlockTime(int height) = 0;

            //! Get block median time past. Height must be valid or this function
            //! will abort.
            virtual int64_t getBlockMedianTimePast(int height) = 0;

            //! Check if we are in process of syncing
            virtual bool isChainSynced() = 0;

            virtual int64_t getBestBlockTime() = 0;

            //! Return height of the first block in the chain with timestamp equal
            //! or greater than the given time, or nullopt if there is no block with
            //! a high enough timestamp. Also return the block hash as an optional
            //! output parameter (to avoid the cost of a second lookup in case this
            //! information is needed.)
            virtual boost::optional<int> findFirstBlockWithTime(int64_t time, uint256* hash) = 0;

            //! Return height of the first block in the chain with timestamp equal
            //! or greater than the given time and height equal or greater than the
            //! given height, or nullopt if there is no such block.
            //!
            //! Calling this with height 0 is equivalent to calling
            //! findFirstBlockWithTime, but less efficient because it requires a
            //! linear instead of a binary search.
            virtual boost::optional<int> findFirstBlockWithTimeAndHeight(int64_t time, int height)
                = 0;
        };

        //! Return Lock interface. Chain is locked when this is called, and
        //! unlocked when the returned interface is freed.
        virtual std::unique_ptr<Lock> lock(bool try_lock = false) = 0;
    };

    struct CoinsView {
        virtual std::vector<COutput> availableCoins(
            Chain::Lock& locked_chain, bool fOnlySafe) const = 0;
        virtual boost::optional<CTxOut> getUTXO(
            Chain::Lock& locked_chain, COutPoint outpoint) const = 0;
        virtual ~CoinsView();
    };

    struct ReserveKeySource {
        virtual boost::optional<CPubKey> getReservedKey() = 0;
        virtual ~ReserveKeySource();
    };
}
}

#endif // INTERFACES_HPP
