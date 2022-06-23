#include "yap/buffer_queue.h"
#include "yap/pipeline.h"

#include <gtest/gtest.h>

#include <numeric>
#include <vector>

namespace
{

struct Iota
{
    explicit Iota(int val = 0) : _val(val)
    {
    }

    int operator()()
    {
        return _val++;
    }

  private:
    int _val;
};

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
}

TEST(TestPipeline, Stop)
{
}

TEST(TestPipeline, Pause)
{
}

TEST(TestPipeline, Consume)
{
    constexpr int nSize = 1'500;

    auto gen = [eng = Iota(1)]() mutable {
        int ret = eng();
        if (ret > nSize)
        {
            throw yap::GeneratorExit{};
        }
        return ret;
    };

    std::vector<int> result, expected(nSize);

    std::iota(expected.begin(), expected.end(), 1);
    result.reserve(nSize);

    // auto pl = yap::Pipeline{} | gen | doubler | halfer | Sink(result);
    auto pp = yap::make_pipeline(gen, doubler, halfer, Sink(result));
    pp->consume();

    EXPECT_EQ(result.size(), expected.size());
    EXPECT_TRUE(result == expected);
}

TEST(TestPipeline, BasicAssertions)
{
    // Expect two strings not to be equal.
    EXPECT_STRNE("hello", "world");
    // Expect equality.
    EXPECT_EQ(7 * 6, 42);
}
