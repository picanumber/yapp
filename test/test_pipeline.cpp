#include "test_common.h"
#include "yap/pipeline.h"

#include <gtest/gtest.h>

#include <atomic>
#include <numeric>
#include <utility>
#include <vector>

namespace
{

struct Sink
{
    explicit Sink(std::vector<int> &output) : _values(output)
    {
    }

    void operator()(int val)
    {
        _values.push_back(val);
    }

  private:
    std::vector<int> &_values;
};

const auto doubler = [](int val) { return 2 * val; };
const auto halfer = [](int val) { return val / 2; };

} // namespace

TEST(TestPipeline, Run)
{
    // auto pl = yap::make_pipeline();
    // TODO(picanumber): Test where output vector is populated
    // using a step controllable with barriers.
}

TEST(TestPipeline, Stop)
{
    unsigned long long counter = 0;
    std::atomic_ullong atomicCounter = 0;
    // Two counters are used, one for parallel access and one intentionally
    // non thread-safe to verify it's not modified in parallel.
    auto mockSink = [&counter, &atomicCounter](auto) {
        ++counter;
        ++atomicCounter;
    };

    auto pl = yap::Pipeline{} | tcn::Iota(1u) | mockSink;
    pl.run();

    while (atomicCounter < tcn::kSmallInputSz)
    {
        // Allow some data to flow.
    }

    // Stop forces the processing threads to exit, which means no further
    // invocation of the pipline methods is possible.
    pl.stop();

    auto acValueOnPause = atomicCounter.exchange(0);
    auto cValueOnPause = std::exchange(counter, 0);

    EXPECT_EQ(acValueOnPause, cValueOnPause);
    EXPECT_GE(acValueOnPause, tcn::kSmallInputSz);
}

TEST(TestPipeline, PauseAndKill)
{
    unsigned long long counter = 0;
    std::atomic_ullong atomicCounter = 0;
    // Two counters are used, one for parallel access and one intentionally
    // non thread-safe to verify it's not modified in parallel.
    auto mockSink = [&counter, &atomicCounter](auto) {
        ++counter;
        ++atomicCounter;
    };

    auto pl = yap::Pipeline{} | tcn::Iota(1u) | mockSink;
    pl.run();

    while (atomicCounter < tcn::kSmallInputSz)
    {
        // Allow some data to flow.
    }

    pl.pause();
    auto acValueOnPause = atomicCounter.exchange(0);
    std::this_thread::sleep_for(1ms);
    EXPECT_LE(atomicCounter.load(), 1ul);

    if (1 == atomicCounter.load())
    {
        // The sink method sneaked an execution after pause.
        // Pause means data exchange between stages is frozen, but if a method
        // is running during pause, this run can finish.
        EXPECT_EQ(acValueOnPause + 1, counter);
    }
}

TEST(TestPipeline, PauseResume)
{
    unsigned long long counter = 0;
    std::atomic_ullong atomicCounter = 0;
    // Two counters are used, one for parallel access and one intentionally
    // non thread-safe to verify it's not modified in parallel.
    auto mockSink = [&counter, &atomicCounter](auto) {
        ++counter;
        ++atomicCounter;
    };

    auto pl = yap::Pipeline{} | tcn::Iota(1u) | mockSink;
    pl.run();

    while (atomicCounter < tcn::kSmallInputSz)
    {
        // Allow some data to flow.
    }

    pl.pause();
    auto acValueOnPause = atomicCounter.exchange(0);
    std::this_thread::sleep_for(1ms);
    EXPECT_LE(atomicCounter.load(), 1ul);

    if (1 == atomicCounter.load())
    {
        // The sink method sneaked an execution after pause.
        // Pause means data exchange between stages is frozen, but if a method
        // is running during pause, this run can finish.
        EXPECT_EQ(acValueOnPause + 1, counter);
    }

    EXPECT_NO_THROW({
        pl.run(); // Resume the pipeline;
        while (atomicCounter < 1'000)
        {
            // Allow some data to flow.
        }
        pl.stop();
    });
}

TEST(TestPipeline, Consume)
{
    auto gen = [eng = tcn::Iota(1)]() mutable {
        int ret = eng();
        if (ret > static_cast<int>(tcn::kMidInputSz))
        {
            throw yap::GeneratorExit{};
        }
        return ret;
    };

    std::vector<int> result, expected(tcn::kMidInputSz);

    std::iota(expected.begin(), expected.end(), 1);
    result.reserve(tcn::kMidInputSz);

    auto pl = yap::Pipeline{} | gen | doubler | halfer | Sink(result);
    pl.consume();

    EXPECT_EQ(result.size(), expected.size());
    EXPECT_TRUE(result == expected);
}

TEST(TestPipeline, ConsumePolymorphic)
{
    auto gen = [eng = tcn::Iota(1)]() mutable {
        int ret = eng();
        if (ret > static_cast<int>(tcn::kMidInputSz))
        {
            throw yap::GeneratorExit{};
        }
        return ret;
    };

    std::vector<int> result, expected(tcn::kMidInputSz);

    std::iota(expected.begin(), expected.end(), 1);
    result.reserve(tcn::kMidInputSz);

    auto pp = yap::make_pipeline(gen, doubler, halfer, Sink(result));
    pp->consume();

    EXPECT_EQ(result.size(), expected.size());
    EXPECT_TRUE(result == expected);
}
