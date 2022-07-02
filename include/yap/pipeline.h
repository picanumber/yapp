// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "buffer_queue.h"
#include "stage.h"
#include "type_utilities.h"

#include <memory>
#include <mutex>
#include <tuple>

namespace yap
{

enum class ReturnValue : uint8_t
{
    Ok,
    Error,
    NoOp
};

/**
 * @brief Parallel data processing pipeline
 * - with one thread per stage,
 * - buffering between stages,
 * - allowing transformations on the data types flowing through.
 *
 * @tparam Ts The <IN,OUT> type transformations imposed on every stage of the
 * pipeline.
 */
class pipeline
{
  public:
    virtual ~pipeline() = default;

    /**
     * @brief Start data processing. Stages will continuously pull from the
     * Generator and pass data to subsequent processors, until the Sink stage is
     * reached.
     *
     * @return A member of the ReturnValue enumeration.
     */
    virtual ReturnValue run() = 0;

    /**
     * @brief Ceases all processing stages and clears data left in the
     * intermediate buffers. Subsequent "run" commands will start fresh.
     *
     * @return A member of the ReturnValue enumeration.
     */
    virtual  ReturnValue stop() = 0;

    /**
     * @brief Ceases all processing stages. Subsequent "run" commands will use
     * any data left in the intermediate buffers.
     *
     * @return A member of the ReturnValue enumeration.
     */
    virtual ReturnValue pause() = 0;

    /**
     * @brief Process all generated data until the generator stage finishes,
     * i.e. throws the "GeneratorExit" exception.
     *
     * @return A member of the ReturnValue enumeration.
     */
    virtual ReturnValue consume() = 0;
};

/**
 * @brief Realization of the parallel data processing pipeline, for arbitrary
 * combinations of data transformations over arbitrary stages.
 *
 * @tparam Ts Deduced during the pipeline construction stages. Users need not
 * specify this typelist. It holds the sequence of <IN,OUT> transformations data
 * is going through over each each stage.
 */
template <class... Ts> class Pipeline : public pipeline
{
    enum class State
    {
        Idle,
        Running,
        Paused
    };

  public:
    Pipeline() = default;
    template <class F, class... Us> Pipeline(Pipeline<Us...> &&pl, F &&fun);
    Pipeline(Pipeline<Ts...> &&other);

    ~Pipeline() override;

    ReturnValue run() override;
    ReturnValue stop() override;
    ReturnValue pause() override;
    ReturnValue consume() override;

  private:
    template <class...> friend class Pipeline;

    ReturnValue runImpl();
    ReturnValue ceaseProcessing();

  private:
    using buffers_t = buffer_list_t<Ts...>;
    using stages_t = stage_list_t<Ts...>;

    buffers_t _buffers;
    stages_t _stages;

    mutable std::mutex _cmdMtx;
    State _state{State::Idle};
};

template <class... Ts>
template <class F, class... Us>
Pipeline<Ts...>::Pipeline(Pipeline<Us...> &&pl, F &&fun)
    : _stages(std::tuple_cat(
          std::move(pl._stages),
          std::make_tuple(
              std::make_unique<typename detail::last_type_impl<
                  stages_t>::type::element_type>(std::forward<F>(fun)))))
{
    if constexpr (std::tuple_size_v<buffers_t>)
    {
        _buffers = std::tuple_cat(
            std::move(pl._buffers),
            std::make_tuple(std::make_shared<typename detail::last_type_impl<
                                buffers_t>::type::element_type>()));
    }
}

template <class... Ts> Pipeline<Ts...>::Pipeline(Pipeline<Ts...> &&other)
{
    std::lock_guard lock(other._cmdMtx);

    _buffers.swap(other._buffers);
    _stages.swap(other._stages);
    std::swap(_state, other._state);
}

template <class... Ts> Pipeline<Ts...>::~Pipeline()
{
    stop();
}

template <class... Ts> ReturnValue Pipeline<Ts...>::runImpl()
{
    auto ret = ReturnValue::NoOp;

    if (State::Idle == _state || State::Paused == _state)
    {
        auto startStages = [this]<std::size_t... Is>(std::index_sequence<Is...>)
        {
            [[maybe_unused]] auto startStage =
                [this]<std::size_t I>(std::integral_constant<std::size_t, I>) {
                    // A running pipeline has at least a generator and a sink.
                    if constexpr (sizeof...(Ts) >= 4)
                    {
                        if constexpr (0 == I)
                        {
                            std::get<0>(_stages)->start(nullptr,
                                                        std::get<0>(_buffers));
                        }
                        else if constexpr (I == std::tuple_size_v<stages_t> - 1)
                        {
                            constexpr auto nBuffers =
                                std::tuple_size_v<buffers_t>;
                            std::get<I>(_stages)->start(
                                std::get<nBuffers - 1>(_buffers), nullptr);
                        }
                        else
                        {
                            std::get<I>(_stages)->start(
                                std::get<I - 1>(_buffers),
                                std::get<I>(_buffers));
                        }
                    }
                    (void)this;
                };

            (startStage(std::integral_constant<std::size_t, Is>{}), ...);
        };

        startStages(std::make_index_sequence<std::tuple_size_v<stages_t>>{});

        _state = State::Running;
        ret = ReturnValue::Ok;
    }

    return ret;
}

template <class... Ts> ReturnValue Pipeline<Ts...>::ceaseProcessing()
{
    auto ret = ReturnValue::NoOp;

    if (State::Running == _state)
    {
        if constexpr (std::tuple_size_v<buffers_t>)
        {
            auto stoppers = std::apply(
                [](auto &...args) { return std::array{args->stop()...}; },
                _stages);

            std::apply(
                [](auto &...args) { (args->set(BufferBehavior::Closed), ...); },
                _buffers);

            ret = ReturnValue::Ok;
        }
    }

    return ret;
}

template <class... Ts> ReturnValue Pipeline<Ts...>::run()
{
    std::lock_guard lk(_cmdMtx);
    return runImpl();
}

template <class... Ts> ReturnValue Pipeline<Ts...>::stop()
{
    std::lock_guard lk(_cmdMtx);

    auto ret = ceaseProcessing();
    if (State::Running == _state || State::Paused == _state)
    {
        std::apply(
            [](auto &...args) {
                ((args->clear(), args->set(BufferBehavior::WaitOnEmpty)), ...);
            },
            _buffers);

        _state = State::Idle;
    }

    return ret;
}

template <class... Ts> ReturnValue Pipeline<Ts...>::pause()
{
    std::lock_guard lk(_cmdMtx);

    auto ret = ceaseProcessing();
    if (State::Running == _state)
    {
        std::apply(
            [](auto &...args) {
                (args->set(BufferBehavior::WaitOnEmpty), ...);
            },
            _buffers);

        _state = State::Paused;
    }

    return ret;
}

template <class... Ts> ReturnValue Pipeline<Ts...>::consume()
{
    auto ret = ReturnValue::NoOp;
    std::lock_guard lk(_cmdMtx);

    if (State::Running != _state)
    {
        ret = runImpl();
    }
    else
    {
        ret = ReturnValue::Ok;
    }

    if (ReturnValue::Ok == ret)
    {
        std::apply([](auto &...args) { (args->consume(), ...); }, _stages);
        _state = State::Idle;
        ret = ReturnValue::Ok;
    }

    return ret;
}

// Deduction guides.
template <class F>
Pipeline(Pipeline<> &&, F &&fun) -> Pipeline<void, std::invoke_result_t<F>>;

template <class F, class... Ts>
Pipeline(Pipeline<Ts...> &&, F &&fun)
    -> Pipeline<Ts...,
                typename std::conditional_t<sizeof...(Ts), last_type<Ts...>,
                                            std::type_identity<void>>::type,
                typename std::conditional_t<
                    sizeof...(Ts), std::invoke_result<F, last_type_t<Ts...>>,
                    std::invoke_result<F>>::type>;

// Chaining operator.
template <class... Ts, class F>
auto operator|(Pipeline<Ts...> &&pl, F &&transform)
{
    return Pipeline(std::move(pl), std::forward<F>(transform));
}

// Abstract base class creator.
template <class... Fs>
std::unique_ptr<pipeline> make_pipeline(Fs &&...transforms)
{
    using pipeline_t = decltype((Pipeline{} | ... | transforms));
    return std::make_unique<pipeline_t>((Pipeline{} | ... | transforms));
}

template <class F> class Copyable
{
    std::shared_ptr<F> _fun;

  public:
    template <class J>
    explicit Copyable(J &&call)
        : _fun(std::make_shared<F>(std::forward<J>(call)))
    {
    }

    template <class... Args> decltype(auto) operator()(Args &&...args)
    {
        return std::invoke(*_fun, std::forward<Args>(args)...);
    }
};

template <class J> Copyable(J &&) -> Copyable<J>;

} // namespace yap
