#ifndef ETHTYPES_HPP
#define ETHTYPES_HPP

#include <boost/multiprecision/cpp_int.hpp>

namespace eth {
using u64 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<64, 64,
    boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;
using u256 = boost::multiprecision::number<boost::multiprecision::cpp_int_backend<256, 256,
    boost::multiprecision::unsigned_magnitude, boost::multiprecision::unchecked, void>>;

template <size_t n> inline u256 exp10()
{
    return exp10<n - 1>() * u256(10);
}

template <> inline u256 exp10<0>()
{
    return u256(1);
}

// The various denominations; here for ease of use where needed within code.
static const u256 ether = exp10<18>();
static const u256 finney = exp10<15>();
static const u256 szabo = exp10<12>();
static const u256 shannon = exp10<9>();
static const u256 wei = exp10<0>();

// Converts from source decimals to target decimals
// Example: 1.5 ETH = 1.5 * 10^18 WEI
// sourceDecimals = 18
// targetDecimals = 8, returns = 1.5 * 10^8
int64_t ConvertFromWeiToSats(
    u256 ethBalance, uint32_t sourceDecimals = 18, uint32_t targetDecimals = 8);

std::string ConvertFromDecimalToHex(eth::u64 value);

u256 ConvertDenominations(u256 value, uint32_t sourceDecimals, uint32_t targetDecimals);

u256 exp10(uint32_t decimals);
}

#endif // ETHTYPES_HPP
