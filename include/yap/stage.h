// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "buffer_queue.h"
#include "runtime_utilities.h"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>

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

template <class IN, class OUT> struct CallConcept
{
    virtual ~CallConcept() = default;
    virtual OUT call(IN) = 0;
};

template <class OUT> struct CallConcept<void, OUT>
{
    virtual ~CallConcept() = default;
    virtual OUT call() = 0;
};

template <typename F, class IN, class OUT>
struct CallModel : CallConcept<IN, OUT>
{
    F f;

    explicit CallModel(F f) : f(std::move(f))
    {
    }

    OUT call(IN arg) override
    {
        return f(std::move(arg));
    }
};

template <typename F, class OUT>
struct CallModel<F, void, OUT> : CallConcept<void, OUT>
{
    F f;

    explicit CallModel(F f) : f(std::move(f))
    {
    }

    OUT call() override
    {
        return f();
    }
};

/**
 * @brief Alternative to std::function, to allow non copyable callables to be
 * used directly in a stage.
 *
 * @tparam IN Input type.
 * @tparam OUT Output type.
 */
template <class IN, class OUT> class Callable
{
    std::unique_ptr<CallConcept<IN, OUT>> impl;

  public:
    Callable() = default;

    template <typename F>
    explicit Callable(F f)
        : impl(std::make_unique<CallModel<F, IN, OUT>>(std::move(f)))
    {
    }

    // Allow moving.
    Callable(Callable &&other) : impl(std::move(other.impl))
    {
    }
    Callable &operator=(Callable &&other)
    {
        impl = std::move(other.impl);
        return *this;
    }

    // Prevent copying.
    Callable(const Callable &) = delete;
    Callable(Callable &) = delete;
    Callable &operator=(const Callable &) = delete;

    // Operate on (possibly move constructed) values.
    template <class J>
    std::enable_if_t<not std::is_void_v<J>, OUT> operator()(J arg)
    {
        return impl->call(std::move(arg));
    }

    template <class J = void>
    std::enable_if_t<std::is_void_v<J>, OUT> operator()()
    {
        return impl->call();
    }
};

// Process a transformation stage. Returns whether to keep processing.
template <class IN, class OUT>
bool process(Callable<IN, OUT> &op,
             std::shared_ptr<BufferQueue<std::future<IN>>> &input,
             std::shared_ptr<BufferQueue<std::future<OUT>>> &output)
{
    try
    {
        // expect not null input/output
        output->push(make_ready_future<OUT>(op(input->pop().get())));
        return true;
    }
    catch (detail::ClosedError &e)
    {
        return false;
    }
    catch (GeneratorExit &e)
    {
        output->push(make_exceptional_future<OUT>(e));
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
bool process(Callable<void, OUT> &op,
             std::shared_ptr<BufferQueue<std::future<void>>> & /*input*/,
             std::shared_ptr<BufferQueue<std::future<OUT>>> &output)
{
    try
    {
        // expect null input, non null output.
        output->push(make_ready_future<OUT>(op()));
        return true;
    }
    catch (detail::ClosedError &e)
    {
        return false;
    }
    catch (GeneratorExit &e)
    {
        output->push(make_exceptional_future<OUT>(e));
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
bool process(Callable<IN, void> &op,
             std::shared_ptr<BufferQueue<std::future<IN>>> &input,
             std::shared_ptr<BufferQueue<std::future<void>>> &
             /*output*/)
{
    try
    {
        // expect null output, non null input.
        op(input->pop().get());
        return true;
    }
    catch (detail::ClosedError &e)
    {
        return false;
    }
    catch (GeneratorExit &e)
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
    detail::Callable<IN, OUT> _operation;
    std::shared_ptr<BufferQueue<std::future<IN>>> _input;
    std::shared_ptr<BufferQueue<std::future<OUT>>> _output;
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

    void start(std::shared_ptr<BufferQueue<std::future<IN>>> input,
               std::shared_ptr<BufferQueue<std::future<OUT>>> output)
    {
        std::lock_guard lk(_cmdMtx);
        if (!_alive)
        {
            _input = std::move(input);
            _output = std::move(output);

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
        if (_alive)
        {
            _worker.join();
            _alive = false;
        }
    }

  private:
    void process()
    {
        while (_alive)
        {
            if (!detail::process(_operation, _input, _output))
            {
                break;
            }
        }
    }
};

} // namespace yap
