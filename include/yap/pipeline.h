// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "buffer_queue.h"
#include "stage.h"
#include "type_utilities.h"

#include <mutex>

namespace yap
{

enum class ReturnValue : uint8_t
{
    Ok,
    Error,
    NoOp
};

class pipeline
{
  public:
    virtual ~pipeline() = default;

    virtual ReturnValue run() = 0;
    virtual ReturnValue stop() = 0;
    virtual ReturnValue pause() = 0;
};

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

    ReturnValue run() override;
    ReturnValue stop() override;
    ReturnValue pause() override;

  private:
    template <class...> friend class Pipeline;

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
}

template <class... Ts> ReturnValue Pipeline<Ts...>::run()
{
    auto ret = ReturnValue::NoOp;
    std::lock_guard lk(_cmdMtx);

    if (State::Idle == _state)
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
                                                        &std::get<0>(_buffers));
                        }
                        else if constexpr (I == std::tuple_size_v<stages_t> - 1)
                        {
                            constexpr auto nBuffers =
                                std::tuple_size_v<buffers_t>;
                            std::get<I>(_stages)->start(
                                &std::get<nBuffers - 1>(_buffers), nullptr);
                        }
                        else
                        {
                            std::get<I>(_stages)->start(
                                &std::get<I - 1>(_buffers),
                                &std::get<I>(_buffers));
                        }
                    }
                };

            (startStage(std::integral_constant<std::size_t, Is>{}), ...);
        };

        startStages(std::make_index_sequence<std::tuple_size_v<stages_t>>{});

        _state = State::Running;
        ret = ReturnValue::Ok;
    }
    else if (State::Paused == _state)
    {
        // TODO: unfreeze buffers

        _state = State::Running;
        ret = ReturnValue::Ok;
    }

    return ret;
}

template <class... Ts> ReturnValue Pipeline<Ts...>::stop()
{
    auto ret = ReturnValue::NoOp;
    std::lock_guard lk(_cmdMtx);

    if (State::Running == _state)
    {
        if constexpr (std::tuple_size_v<buffers_t>)
        {
            // Stop stage workers. A stage pulling from an empty input freezes.
            auto stoppers = std::apply(
                [](auto &...args) { return std::array{args->stop()...}; },
                _stages);

            // Buffers will throw when attempting to pull from an empty queue.
            // Waiting stages wake up and realize they have to die.
            std::apply(
                [](auto &...args) {
                    (args.setPop(PopBehavior::ThrowOnEmpty), ...);
                },
                _buffers);

            for (auto &stop : stoppers)
            {
                stop.get(); // Wait for stoppers to complete.
            }

            _state = State::Idle;
            ret = ReturnValue::Ok;
        }
    }
    else if (State::Paused == _state)
    {
        // TODO: Implement

        _state = State::Idle;
        ret = ReturnValue::Ok;
    }

    // TODO: Empty buffers .

    return ret;
}

template <class... Ts> ReturnValue Pipeline<Ts...>::pause()
{
    auto ret = ReturnValue::NoOp;
    std::lock_guard lk(_cmdMtx);

    if (_state == State::Running)
    {
        // TODO

        _state = State::Paused;
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

} // namespace yap
