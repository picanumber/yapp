// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "buffer_queue.h"

#include <atomic>
#include <functional>
#include <future>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace yap
{

/**
 * @brief Create a future that already contains a result.
 *
 * @tparam T Type to wrap into the future.
 * @tparam Args Types of the arguments passed to T's constructor.
 * @param ...args Arguments to forward into T's constructor.
 *
 * @return A future that contains a value T{args...} or void.
 */
template <class T, class... Args>
std::future<T> make_ready_future(Args &&...args)
{
    std::promise<T> p;
    if constexpr (std::is_void_v<T>)
    {
        static_assert(0 == sizeof...(Args),
                      "future<void> constructible without arguments");
        p.set_value();
    }
    else
    {
        p.set_value(T{std::forward<Args>(args)...});
    }

    return p.get_future();
}

/**
 * @brief Create a future that contains an exception.
 *
 * @tparam T Type to wrap into the future.
 * @tparam E Type of the exception to store
 * @param ex Exception to store in the future.
 *
 * @return A future that contains the exception ex.
 */
template <class T, class E> std::future<T> make_exceptional_future(E ex)
{
    std::promise<T> p;
    p.set_exception(std::make_exception_ptr(std::move(ex)));

    return p.get_future();
}

namespace detail
{

template <class IN, class OUT> struct Function
{
    using type = std::function<OUT(IN)>;
};
template <class OUT> struct Function<void, OUT>
{
    using type = std::function<OUT()>;
};
template <class IN, class OUT>
using function_t = typename Function<IN, OUT>::type;

// Process a transformation stage. Returns whether to keep processing.
template <class IN, class OUT>
bool process(std::function<OUT(IN)> &op, BufferQueue<std::future<IN>> *input,
             BufferQueue<std::future<OUT>> *output)
{
    try
    {
        // expect not null input/output
        output->push(make_ready_future<OUT>(op(input->pop().get())));
        return true;
    }
    catch (ClosedError &e)
    {
        return false;
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
        return true;
    }
}

// Process a generator stage.
template <class OUT>
bool process(std::function<OUT()> &op,
             BufferQueue<std::future<void>> * /*input*/,
             BufferQueue<std::future<OUT>> *output)
{
    try
    {
        // expect null input, non null output.
        output->push(make_ready_future<OUT>(op()));
        return true;
    }
    catch (ClosedError &e)
    {
        return false;
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
        return true;
    }
}

// Process a sink stage.
template <class IN>
bool process(std::function<void(IN)> &op, BufferQueue<std::future<IN>> *input,
             BufferQueue<std::future<void>> * /*output*/)
{
    try
    {
        // expect null output, non null input.
        op(input->pop().get());
        return true;
    }
    catch (ClosedError &e)
    {
        return false;
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
        return true;
    }
}

} // namespace detail

template <class IN, class OUT> class Stage
{
    detail::function_t<IN, OUT> _operation;
    BufferQueue<std::future<IN>> *_input = nullptr;
    BufferQueue<std::future<OUT>> *_output = nullptr;
    std::thread _worker;
    std::mutex _cmdMtx;
    std::atomic_bool _alive{false};

  public:
    template <class F>
    explicit Stage(F &&operation) : _operation(std::forward<F>(operation))
    {
    }

    ~Stage()
    {
        stop();
    }

    void start(BufferQueue<std::future<IN>> *input,
               BufferQueue<std::future<OUT>> *output)
    {
        std::lock_guard lk(_cmdMtx);
        if (!_alive)
        {
            _input = input;
            _output = output;

            _alive = true;
            _worker = std::thread(&Stage::process, this);
        }
    }

    auto stop()
    {
        return std::async([this] {
            std::lock_guard lk(_cmdMtx);
            if (_alive)
            {
                _alive = false;
                _worker.join();
            }
        });
    }

    // Let the worker run, until it exits due to ClosedBuffer error.
    void consume()
    {
        std::lock_guard lk(_cmdMtx);
        _worker.join();
        _alive = false;
    }

  private:
    void process()
    {
        while (_alive)
        {
            if (!detail::process(_operation, _input, _output))
            {
                // The worker is considered alive, since it's not joined. After
                // such a bail-out, only "stop()" can be called upon the
                // instance, or "consume()".
                break;
            }
        }
    }
};

} // namespace yap
