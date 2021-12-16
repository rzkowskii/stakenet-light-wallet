#ifndef TST_REGTEST_HPP
#define TST_REGTEST_HPP

// Copyright (c) %YEAR The XSN developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include <Chain/RegtestChain.hpp>
#include <gtest/gtest.h>
#include <script/script.h>
#include <utilstrencodings.h>

using namespace testing;

#if 0

TEST(RegtestTests, GenesisCheck)
{
    RegtestChain chain;
    chain.load();
    ASSERT_GT(chain.height(), 0);
    ASSERT_EQ(chain.bestBlockHash(), "0f9188f13cb7b2c71f2a335e3a4fc328bf5beb436012afca590b1a11466e2206");
}

TEST(RegtestTests, Generate5Blocks)
{
    RegtestChain chain;
    chain.load();
    int oldHeight = chain.height();
    int expectedHeight = oldHeight + 5;
    chain.generateBlocks(bitcoin::CScript(), expectedHeight - oldHeight);
    ASSERT_EQ(chain.height(), expectedHeight);
}

TEST(RegtestTests, CheckSequence)
{
    RegtestChain chain;
    chain.load();
    chain.generateBlocks(bitcoin::CScript(), 10);

    auto prevHash = GetHashStr(chain.blockAt(0).header);
    for(size_t i = 1; i < chain.height(); ++i)
    {
        ASSERT_EQ(chain.blockAt(i).header.prevBlock, prevHash);
        prevHash = GetHashStr(chain.blockAt(i).header);
    }
}

#endif

#endif // TST_REGTEST_HPP
