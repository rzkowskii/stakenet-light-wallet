#include "Types.hpp"
#include <EthCore/Types.hpp>

#include <QDebug>
#include <cmath>
#include <tinyformat.h>

namespace swaps {

//==============================================================================

static constexpr int64_t COIN = 100000000;

//==============================================================================

MakerTakerAmounts MakerTakerAmounts::CalculateMakerTakerAmounts(
    int64_t quantity, int64_t price, bool isBuy, std::string pairId)
{
    MakerTakerAmounts amounts;
    auto pairIdSplit = QString::fromStdString(pairId).split('_');
    auto baseCurrency = pairIdSplit.at(0).toStdString();
    auto quoteCurrency = pairIdSplit.at(1).toStdString();

    double priceAsAmount = static_cast<double>(price) / COIN;

    // we pass values in satoshis
    auto baseCurrencyAmount = quantity;
    // price and quantity are both in sats so to get total we need to: quantity / price
    auto quoteCurrencyAmount
        = static_cast<int64_t>(price > 0 && price < std::numeric_limits<decltype(price)>::max()
                ? static_cast<double>(quantity) * priceAsAmount
                : 0); // if price is zero or infinity, this is a market order and we can't know the
                      // quote currency amount

    auto baseCurrencyUnits = eth::ConvertDenominations(
        eth::u256{ baseCurrencyAmount }, 8, UNITS_PER_CURRENCY.at(baseCurrency));
    auto quoteCurrencyUnits = eth::ConvertDenominations(
        eth::u256{ quoteCurrencyAmount }, 8, UNITS_PER_CURRENCY.at(quoteCurrency));

    amounts.maker.currency = isBuy ? baseCurrency : quoteCurrency;
    amounts.maker.amount = isBuy ? baseCurrencyAmount : quoteCurrencyAmount;
    amounts.maker.units = isBuy ? baseCurrencyUnits : quoteCurrencyUnits;
    amounts.taker.currency = isBuy ? quoteCurrency : baseCurrency;
    amounts.taker.amount = isBuy ? quoteCurrencyAmount : baseCurrencyAmount;
    amounts.taker.units = isBuy ? quoteCurrencyUnits : baseCurrencyUnits;

    return amounts;
}

//==============================================================================

int64_t MakerTakerAmounts::CalculateOrderAmount(int64_t total, int64_t price, std::string pairId)
{
    auto pairIdSplit = QString::fromStdString(pairId).split('_');
    auto quoteCurrency = pairIdSplit.at(1).toStdString();

    double priceAsCoins = static_cast<double>(price) / COIN;
    return static_cast<int64_t>(price > 0 && price < std::numeric_limits<decltype(price)>::max()
            ? std::round(static_cast<double>(total) / priceAsCoins)
            : 0);
}

//==============================================================================

std::string MakerTakerAmounts::toString() const
{
    return strprintf(
        "taker: %lu %s, maker: %lu %s", taker.units, taker.currency, maker.units, maker.currency);
}

//==============================================================================
}
