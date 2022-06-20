// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace yap
{

enum class PopBehavior : uint8_t
{
    Frozen,       // Block all pop operations.
    ThrowOnEmpty, // Never block on pop, throws if popping from an empty queue.
    WaitOnEmpty   // Block until an element is available to pop.
};

namespace detail
{

/**
 * @brief Used internally. User code is not allowed to throw this exception.
 */
struct EmptyInput : std::logic_error
{
    EmptyInput() : std::logic_error("EmptyInput")
    {
    }
};

} // namespace detail

/**
 * @brief Throw this exception to signal the end of available input.
 */
struct FinishedInput : std::logic_error
{
    FinishedInput() : std::logic_error("FinishedInput")
    {
    }
};

template <class T> class BufferQueue final
{
    std::deque<T> _contents;
    mutable std::mutex _mtx;
    mutable std::condition_variable _bell;
    std::atomic<PopBehavior> _popCondition{PopBehavior::WaitOnEmpty};

  public:
    void clear()
    {
        {
            std::lock_guard lk(_mtx);
            _contents.clear();
        }
        _bell.notify_all();
    }

    template <class... Args> void push(Args &&...args)
    {
        {
            std::unique_lock lk(_mtx);
            _bell.wait(lk,
                       [this] { return _popCondition != PopBehavior::Frozen; });
            _contents.emplace_back(std::forward<Args>(args)...);
        }
        _bell.notify_all();
    }

    auto pop()
    {
        std::unique_lock lk(_mtx);
        _bell.wait(lk, [this] {
            switch (_popCondition)
            {
            case PopBehavior::Frozen:
                return false;
            case PopBehavior::ThrowOnEmpty:
                return true;
            case PopBehavior::WaitOnEmpty:
                return !_contents.empty();
            default:
                // TODO: expects here.
                return true;
            }
        });

        try
        {
            auto ret{std::move(_contents.at(0))};
            _contents.pop_front();
            return ret;
        }
        catch (std::out_of_range &e)
        {
            throw detail::EmptyInput{};
        }
    }

    void setPop(PopBehavior val)
    {
        _popCondition.store(val);
        _bell.notify_all();
    }
};

// A void queue, is an empty but valid class.
template <> class BufferQueue<void>
{
};

} // namespace yap
