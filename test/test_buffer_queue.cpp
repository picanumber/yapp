#include "yap/pipeline.h"

#include <gtest/gtest.h>

TEST(TestBufferQueue, BasicAssertions)
{
    double *var = new double;
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}
