#include "AbstractSwapRepository.hpp"
#include <Utils/Logging.hpp>

namespace swaps {

//==============================================================================

AbstractSwapRepository::AbstractSwapRepository(QObject* parent)
    : QObject(parent)
{
}

//==============================================================================

void AbstractSwapRepository::load()
{
    try {
        _deals = executeLoadAll();
    } catch (std::exception& ex) {
        LogCCritical(Swaps) << "Failed to load deals from database" << ex.what();
    }
}

//==============================================================================

void AbstractSwapRepository::saveDeal(SwapDeal deal)
{
    if (this->deal(deal.rHash)) {
        updateDeal(deal);
    } else {
        executeSaveDeal(deal);
        _deals.emplace_back(deal);
        dealAdded(deal);
    }
}

//==============================================================================

void AbstractSwapRepository::updateDeal(SwapDeal deal)
{
    auto it = std::find_if(std::begin(_deals), std::end(_deals),
        [rHash = deal.rHash](const auto& deal) { return deal.rHash == rHash; });

    if (it != std::end(_deals)) {
        executeSaveDeal(deal);
        *it = deal;
        dealChanged(deal);
    }
}

//==============================================================================

boost::optional<SwapDeal> AbstractSwapRepository::deal(std::string rHash) const
{
    auto it = std::find_if(std::begin(_deals), std::end(_deals),
        [rHash](const auto& deal) { return deal.rHash == rHash; });

    return it != std::end(_deals) ? boost::make_optional(*it) : boost::none;
}

//==============================================================================

auto swaps::AbstractSwapRepository::deals() const -> const Deals&
{
    return _deals;
}

//==============================================================================
}
