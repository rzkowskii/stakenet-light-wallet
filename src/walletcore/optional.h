// Copyright (c) 2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_OPTIONAL_H
#define BITCOIN_OPTIONAL_H

#include <boost/optional.hpp>
#include <utility>

////! Substitute for c++14 std::optional
//template <typename T> using Optional = boost::optional<T>;

//! Substitute for c++14 std::make_optional
template <typename T>
boost::optional<T> MakeOptional(bool condition, T&& value)
{
    return boost::make_optional(condition, std::forward<T>(value));
}

//! Substitute for c++14 std::nullopt
static auto& nullopt = boost::none;

#endif // BITCOIN_OPTIONAL_H
