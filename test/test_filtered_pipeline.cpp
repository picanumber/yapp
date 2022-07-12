#include "test_common.h"
#include "yap/pipeline.h"

#include <gtest/gtest.h>

#include <chrono>
#include <numeric>
#include <optional>
#include <thread>
#include <vector>

namespace
{

auto intMaker = [val = 0]() mutable { return val++; };

auto onlyOdds = [](int val) -> std::optional<int> {
    return val % 2 ? std::optional<int>(val) : std::nullopt;
};

auto onlyEvens = [](int val) -> std::optional<int> {
    return val % 2 ? std::nullopt : std::optional<int>(val);
};

auto sinker = [](auto &v) {
    return [&v](auto oi) { v.push_back(oi.value()); };
};

} // namespace

TEST(TestFilteredPipeline, FilterContainer)
{
    std::vector<int> dest;
    std::vector<int> source(tcn::kMidInputSz);

    std::iota(source.begin(), source.end(), -1'000);

    auto odder = yap::Pipeline{} | yap::Consume(source.begin(), source.end()) |
                 onlyOdds | sinker(dest);

    odder.consume(); // Run the pipeline until all input is consumed.

    EXPECT_EQ(dest.size(), 0.5 * source.size());
    for (int val : dest)
    {
        EXPECT_NE(0, val % 2);
    }
}

TEST(TestFilteredPipeline, FilterGenerated)
{
    std::vector<int> dest;
    auto odder = yap::Pipeline{} | intMaker | onlyEvens | sinker(dest);

    odder.run();
    std::this_thread::sleep_for(1ms);
    odder.stop();

    for (int val : dest)
    {
        EXPECT_EQ(0, val % 2);
    }
}
