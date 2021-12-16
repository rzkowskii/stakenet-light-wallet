#ifndef DBUTILS_HPP
#define DBUTILS_HPP

#include <Chain/BlockHeader.hpp>
#include <Chain/Chain.hpp>
#include <Data/TransactionEntry.hpp>
#include <txdb.h>
#include <utilstrencodings.h>

namespace DBUtils {
bitcoin::CDiskBlockIndex BlockIndexToDisk(const Wire::VerboseBlockHeader& index);
Wire::VerboseBlockHeader DiskIndexToBlockIndex(const bitcoin::CDiskBlockIndex& diskIndex);
}

#endif // DBUTILS_HPP
