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

/**
 * @brief Raised when a generator or coroutine is closed.
 */
struct GeneratorExit : std::logic_error
{
    GeneratorExit() : std::logic_error("GeneratorExit")
    {
    }
};

template <class T> class BufferQueue final
{
    struct State
    {
        std::deque<T> contents;
        mutable std::mutex mtx;
        mutable std::condition_variable bell;
        std::atomic<BufferBehavior> popCondition{BufferBehavior::WaitOnEmpty};
    };
    std::unique_ptr<State> _state = std::make_unique<State>();

  public:
    void clear()
    {
        {
            std::lock_guard lk(_state->mtx);
            _state->contents.clear();
        }
        _state->bell.notify_all();
    }

    template <class... Args> void push(Args &&...args)
    {
        {
            std::unique_lock lk(_state->mtx);
            _state->bell.wait(lk, [this] {
                return _state->popCondition != BufferBehavior::Frozen;
            });

            if (BufferBehavior::Closed == _state->popCondition)
            {
                throw detail::ClosedError(false);
            }

            _state->contents.emplace_back(std::forward<Args>(args)...);
        }
        _state->bell.notify_all();
    }

    auto pop()
    {
        std::unique_lock lk(_state->mtx);
        _state->bell.wait(lk, [this] {
            switch (_state->popCondition)
            {
            case BufferBehavior::Closed:
                return true;
            case BufferBehavior::WaitOnEmpty:
                return !_state->contents.empty();
            case BufferBehavior::Frozen:
            default: // TODO: expects here.
                return false;
            }
        });

        if (BufferBehavior::Closed == _state->popCondition)
        {
            throw detail::ClosedError(true);
        }

        auto ret{std::move(_state->contents.at(0))};
        _state->contents.pop_front();
        return ret;
    }

    void set(BufferBehavior val)
    {
        {
            std::lock_guard lk(_state->mtx);
            _state->popCondition.store(val);
        }
        _state->bell.notify_all();
    }
};

// A void queue, is an empty but valid class.
template <> class BufferQueue<void>
{
};

} // namespace yap
