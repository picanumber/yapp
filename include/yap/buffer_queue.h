// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include <atomic>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>

namespace yap
{

enum class BufferBehavior : uint8_t
{
    Frozen,      // Block all pop operations.
    WaitOnEmpty, // Block until an element is available to pop.
    Closed       // No IO can be performed on the buffer.
};

namespace detail
{

/**
 * @brief Internally thrown when IO is attempted on a closed buffer.
 */
struct ClosedError : std::logic_error
{
    explicit ClosedError(bool onPop)
        : std::logic_error(onPop ? "No data can be popped from the buffer"
                                 : "No data can be pushed to the buffer")
    {
    }
};

} // namespace detail

template <class T> class BufferQueue final
{
    std::deque<T> _contents;
    mutable std::mutex _mtx;
    mutable std::condition_variable _bell;
    BufferBehavior _popCondition{BufferBehavior::WaitOnEmpty};

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
            _bell.wait(
                lk, [this] { return _popCondition != BufferBehavior::Frozen; });

            if (BufferBehavior::Closed == _popCondition)
            {
                throw detail::ClosedError(false);
            }

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
            case BufferBehavior::Closed:
                return true;
            case BufferBehavior::WaitOnEmpty:
                return !_contents.empty();
            case BufferBehavior::Frozen:
            default: // TODO: expects here.
                return false;
            }
        });

        if (BufferBehavior::Closed == _popCondition)
        {
            throw detail::ClosedError(true);
        }

        auto ret{std::move(_contents.at(0))};
        _contents.pop_front();
        return ret;
    }

    void set(BufferBehavior val)
    {
        {
            std::lock_guard lk(_mtx);
            _popCondition = val;
        }
        _bell.notify_all();
    }
};

// A void queue, is an empty but valid class.
template <> class BufferQueue<void>
{
};

} // namespace yap
