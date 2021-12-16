#ifndef TST_ADDRESSMANAGER_HPP
#define TST_ADDRESSMANAGER_HPP

#include <gtest/gtest.h>
#include <iostream>

using namespace testing;

class AddressManagerTest : public ::testing::Test {
protected:
    virtual void SetUp() {}

    virtual void TearDown() {}
};

TEST_F(AddressManagerTest, startsync) {}

#endif // TST_ADDRESSMANAGER_HPP
