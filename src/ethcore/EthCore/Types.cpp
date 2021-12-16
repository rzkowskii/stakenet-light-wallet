#include "Types.hpp"


//==============================================================================

static std::vector<eth::u256> decimalsCache;

//==============================================================================

int64_t eth::ConvertFromWeiToSats(
    eth::u256 ethBalance, uint32_t sourceDecimals, uint32_t targetDecimals)
{
    return ConvertDenominations(ethBalance, sourceDecimals, targetDecimals).convert_to<int64_t>();
}

//==============================================================================

eth::u256 eth::exp10(uint32_t decimals)
{
    if (decimals < decimalsCache.size()) {
        return decimalsCache.at(decimals);
    }

    auto value = decimals == 0 ? u256(1) : exp10(decimals - 1) * u256(10);
    decimalsCache.push_back(value);
    return value;
}

//==============================================================================

eth::u256 eth::ConvertDenominations(
    eth::u256 value, uint32_t sourceDecimals, uint32_t targetDecimals)
{
    if (sourceDecimals > targetDecimals) {
        value /= eth::exp10(sourceDecimals - targetDecimals);
    } else if (sourceDecimals < targetDecimals) {
        value *= eth::exp10(targetDecimals - sourceDecimals);
    }
    return value;
}

//==============================================================================

std::string eth::ConvertFromDecimalToHex(eth::u64 value)
{
    std::stringstream ss;
    ss << std::hex << std::showbase << value;
    return ss.str();
}

//==============================================================================
