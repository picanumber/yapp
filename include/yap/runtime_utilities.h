// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include <future>
#include <stdexcept>

namespace yap
{

/**
 * @brief Raised when a generator is closed.
 */
struct GeneratorExit : std::logic_error
{
    GeneratorExit() : std::logic_error("GeneratorExit")
    {
    }
};

template <class It> class Consume
{
    It _begin;
    It _end;

  public:
    Consume(It begin, It end) : _begin(begin), _end(end)
    {
    }

    // This will not produce a reference. A consume target is either copied (by
    // iterators) or moved (by move iterators). Working with references within
    // the pipeline is not facilitated to avoid data races.
    auto operator()()
    {
        if (_begin != _end)
        {
            return *_begin++;
        }
        throw GeneratorExit{};
    }
};

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

template <class F, class IN, class OUT> struct CallModel : CallConcept<IN, OUT>
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

template <class F, class OUT>
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

} // namespace detail

/**
 * @brief Alternative to std::function, to allow non copyable callables to be
 * used directly in a stage.
 *
 * @tparam IN Input type.
 * @tparam OUT Output type.
 */
template <class IN, class OUT> class Callable
{
    std::unique_ptr<detail::CallConcept<IN, OUT>> _impl;

  public:
    Callable() = default;

    template <class F>
    explicit Callable(F f)
        : _impl(std::make_unique<detail::CallModel<F, IN, OUT>>(std::move(f)))
    {
    }

    template <class J>
    std::enable_if_t<not std::is_void_v<J>, OUT> operator()(J arg)
    {
        return _impl->call(std::move(arg));
    }

    template <class J = void>
    std::enable_if_t<std::is_void_v<J>, OUT> operator()()
    {
        return _impl->call();
    }
};

} // namespace yap
