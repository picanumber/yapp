#include "test_common.h"
#include "yap/stage.h"

#include <gtest/gtest.h>

#include <future>
#include <utility>
#include <vector>

TEST(TestStage, CheckGenerator)
{
    auto outQ = std::make_shared<yap::BufferQueue<std::future<int>>>();

    yap::Stage<void, int> generatorStage([seq = tcn::Iota(1u)]() mutable {
        auto ret = seq();
        if (ret > tcn::kMidInputSz)
        {
            throw yap::GeneratorExit{};
        }
        return ret;
    });

    generatorStage.start(nullptr, outQ);

    auto val{0u}, expected{1u};
    while (tcn::kMidInputSz != val)
    {
        val = outQ->pop().get();
        EXPECT_EQ(expected++, val);
    }

    generatorStage.consume();

    // Queue should contain the "exit" exception.
    EXPECT_THROW(outQ->pop().get(), yap::GeneratorExit);

    // Should block since queue is and will remain empty.
    auto try_pop = std::async([&] { return outQ->pop(); });
    auto ret = try_pop.wait_for(1ms);
    EXPECT_EQ(ret, std::future_status::timeout);

    outQ->set(yap::BufferBehavior::Closed);
    EXPECT_THROW(try_pop.get(), yap::detail::ClosedError);

    EXPECT_NO_THROW(generatorStage.stop())
        << "Stopping a consumed stage should not throw";
}

TEST(TestStage, CheckSink)
{
    auto inQ = std::make_shared<yap::BufferQueue<std::future<int>>>();
    for (std::size_t i(1); i <= tcn::kMidInputSz; ++i)
    {
        inQ->push(yap::make_ready_future<int>(static_cast<int>(i)));
    }
    inQ->push(yap::make_exceptional_future<int>(yap::GeneratorExit{}));

    std::vector<int> humble, nonHumble, allNumbers;
    yap::Stage<int, void> sinkStage(
        [&humble, &nonHumble, &allNumbers](int val) {
            allNumbers.push_back(val);
            if (tcn::is_humble(val))
            {
                humble.push_back(val);
            }
            else
            {
                nonHumble.push_back(val);
            }
        });

    sinkStage.start(inQ, nullptr);
    sinkStage.consume(); // No further IO should be done after this call.

    int expected{0};
    for (int val : allNumbers)
    {
        EXPECT_EQ(val, ++expected);
    }
    EXPECT_EQ(expected, tcn::kMidInputSz);

    EXPECT_EQ(tcn::kMidInputSz, humble.size() + nonHumble.size());
    for (int val : humble)
    {
        EXPECT_TRUE(tcn::is_humble(val));
    }
    for (int val : nonHumble)
    {
        EXPECT_FALSE(tcn::is_humble(val));
    }

    // Should block since queue is and will remain empty.
    auto try_pop = std::async([&] { return inQ->pop(); });
    auto ret = try_pop.wait_for(1ms);
    EXPECT_EQ(ret, std::future_status::timeout);

    inQ->set(yap::BufferBehavior::Closed);
    EXPECT_THROW(try_pop.get(), yap::detail::ClosedError);

    EXPECT_NO_THROW(sinkStage.stop())
        << "Stopping a consumed stage should not throw";
}

TEST(TestStage, CheckTransformation)
{
    auto inQ = std::make_shared<yap::BufferQueue<std::future<int>>>();
    auto outQ =
        std::make_shared<yap::BufferQueue<std::future<std::pair<int, bool>>>>();

    for (std::size_t i(1); i <= tcn::kMidInputSz; ++i)
    {
        inQ->push(yap::make_ready_future<int>(static_cast<int>(i)));
    }
    inQ->push(yap::make_exceptional_future<int>(yap::GeneratorExit{}));

    yap::Stage<int, std::pair<int, bool>> transformStage(
        [](int val) { return std::make_pair(val, tcn::is_humble(val)); });

    transformStage.start(inQ, outQ);

    auto val{0u}, expected{1u};
    while (tcn::kMidInputSz != val)
    {
        auto [cur, isHumble] = outQ->pop().get();
        EXPECT_EQ(expected, cur);
        EXPECT_EQ(isHumble, tcn::is_humble(expected));

        ++expected;
        val = cur;
    }

    transformStage.consume(); // No further IO should be done after this call.
    EXPECT_THROW(outQ->pop().get(), yap::GeneratorExit);

    // Should block since queue is and will remain empty.
    auto try_pop = std::async([&] { return outQ->pop(); });
    auto ret = try_pop.wait_for(1ms);
    EXPECT_EQ(ret, std::future_status::timeout);

    outQ->set(yap::BufferBehavior::Closed);
    EXPECT_THROW(try_pop.get(), yap::detail::ClosedError);

    EXPECT_NO_THROW(transformStage.stop())
        << "Stopping a consumed stage should not throw";
}
