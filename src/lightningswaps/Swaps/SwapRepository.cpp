#include "SwapRepository.hpp"
#include <boost/thread.hpp>
#include <dbwrapper.h>

namespace swaps {

static const char DB_DEALS_INDEX{ 'd' };

//==============================================================================

struct SwapRepository::DBImpl : public bitcoin::CDBWrapper {
    DBImpl(std::string dataDir, size_t nCacheSize, bool fMemory, bool fWipe)
        : bitcoin::CDBWrapper(dataDir, nCacheSize, fMemory, fWipe)
    {
    }
};

//==============================================================================

SwapRepository::SwapRepository(std::string dataDir, QObject* parent)
    : AbstractSwapRepository(parent)
    , _db(new DBImpl(dataDir, 1000, false, false))
{
}

//==============================================================================

SwapRepository::~SwapRepository() {}

//==============================================================================

AbstractSwapRepository::Deals SwapRepository::executeLoadAll()
{
    using namespace bitcoin;
    Deals result;
    std::unique_ptr<CDBIterator> pcursor(_db->NewIterator());
    std::pair<char, std::string> key;
    pcursor->Seek(DB_DEALS_INDEX);

    // Load mapBlockIndex
    while (pcursor->Valid()) {
        boost::this_thread::interruption_point();
        if (pcursor->GetKey(key) && key.first == DB_DEALS_INDEX) {
            SwapDeal deal;
            if (pcursor->GetValue(deal)) {
                result.emplace_back(deal);
                pcursor->Next();
            } else {
                throw std::runtime_error("Failed to read swap repository value");
            }
        } else {
            break;
        }
    }

    return result;
}

//==============================================================================

void SwapRepository::executeSaveDeal(SwapDeal deal)
{
    _db->Write(std::make_pair(DB_DEALS_INDEX, deal.rHash), deal);
}

//==============================================================================
}
