#include "test_common.h"
#include "yap/pipeline.h"
#include "yap/topology.h"

#include <gtest/gtest.h>

#include <numeric>
#include <optional>
#include <thread>
#include <vector>

namespace
{

auto sinker = [](auto &v) {
    return [&v](auto oi) { v.push_back(oi.value()); };
};

// An input number N produces N number of N values, e.g.:
// 1 -> 1
// 2 -> 2, 2
// 3 -> 3, 3, 3
// 4 -> 4, 4, 4, 4
auto intHatching = [curVal = 0, count = 0](yap::Hatchable<int> val) mutable {
    std::optional<int> ret;

    if (val)
    {
        // Input from previous stage.
        if (*val.data > 0)
        {
            curVal = *val.data;
            count = curVal;
            ret.emplace(curVal);
        }
    }
    else
    {
        // Output hatching (keep processing the last input).
        if (--count)
        {
            ret.emplace(curVal);
        }
    }

    return ret;
};

} // namespace

TEST(TestHatchingPipeline, NumberGeneratingStage)
{
    std::vector<int> dest;
    std::vector<int> source{1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

    auto odder =
        yap::Pipeline{} |
        yap::OutputHatchable(yap::Consume(source.begin(), source.end())) |
        intHatching | sinker(dest);

    odder.consume(); // Run the pipeline until all input is consumed.

    std::size_t expectedDestinationSize{0};
    for (int val : source)
    {
        expectedDestinationSize += val;
    }
    EXPECT_EQ(dest.size(), expectedDestinationSize);

    // Verify that each number is repeated as many times as its value, e.g.
    // there are 3 threes, 4 fours, 5 fives and so on.
    for (std::size_t i(0); i < dest.size();)
    {
        int curVal = dest[i];
        for (int j(0); j < curVal; ++j)
        {
            EXPECT_EQ(dest[i + j], curVal);
        }

        i += curVal;
    }
}
