#include "test_common.h"
#include "yap/buffer_queue.h"
#include "yap/pipeline.h"

#include <gtest/gtest.h>

#include <future>

TEST(TestBufferQueue, CheckFifoBehavior)
{
    using buffer_t = yap::BufferQueue<tcn::MoveOnly<int>>;

    buffer_t buf;
    auto fill = std::async([&buf] {
        for (std::size_t i = 1; i <= tcn::kMidInputSz; ++i)
        {
            buf.push(i);
        }
    });

    std::size_t lastValue = 0;
    while (++lastValue)
    {
        EXPECT_EQ(lastValue, buf.pop().get());
        if (lastValue == tcn::kMidInputSz)
        {
            break;
        }
    }
    fill.get();
}

TEST(TestBufferQueue, CheckClose)
{
    using buffer_t = yap::BufferQueue<int>;

    buffer_t buf;
    buf.set(yap::BufferBehavior::Closed);

    try
    {
        buf.push(1);
        FAIL() << "Pushing to a closed buffer should throw";
    }
    catch (yap::detail::ClosedError &e)
    {
    }
    catch (...)
    {
        FAIL() << "Incorrect exception emitted";
    }

    try
    {
        buf.pop();
        FAIL() << "Popping from a closed buffer should throw";
    }
    catch (yap::detail::ClosedError &e)
    {
    }
    catch (...)
    {
        FAIL() << "Incorrect exception emitted";
    }

    buf.set(yap::BufferBehavior::WaitOnEmpty);
    buf.push(1);
    EXPECT_EQ(1, buf.pop());
}

TEST(TestBufferQueue, CheckClear)
{
    using buffer_t = yap::BufferQueue<int>;

    buffer_t buf;
    buf.push(1);
    buf.push(2);
    buf.push(3);

    buf.clear();
    auto try_pop = std::async([&buf] { return buf.pop(); });

    auto status = try_pop.wait_for(1ms);
    EXPECT_EQ(std::future_status::timeout, status);

    buf.push(23);
    EXPECT_EQ(23, try_pop.get());
}

TEST(TestBufferQueue, CheckFrozen)
{
    using buffer_t = yap::BufferQueue<int>;
    int value = 1;

    buffer_t buf;
    buf.push(value);

    buf.set(yap::BufferBehavior::Frozen);
    auto try_pop = std::async([&buf] { return buf.pop(); });

    auto status = try_pop.wait_for(1ms);
    EXPECT_EQ(std::future_status::timeout, status);

    buf.set(yap::BufferBehavior::WaitOnEmpty);
    EXPECT_EQ(value, try_pop.get());
}
