// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "runtime_utilities.h"

#include <functional>
#include <optional>

namespace yap
{

/**
 * @brief Designates objects produced from a filtering stage.
 *
 * @details A filtering stage is one that processes N number of input objects
 * and produces <=N output objects. When a filtering stage ingests an input I
 * and produces an object with no value (empty optional), subsequent stages
 * ignore the data flow that started with I.
 *
 * @tparam T Type of the stored data.
 */
template <class T> struct Filtered
{
    std::optional<T> data;

    Filtered() = default;
    explicit Filtered(T &&data) : data(std::move(data))
    {
    }
    explicit Filtered(std::optional<T> &&data) : data(std::move(data))
    {
    }
};

template <class F> auto Filter(F &&operation)
{
    return [op = std::forward<F>(operation)](auto &&...args) mutable {
        return Filtered(std::invoke(op, std::forward<decltype(args)>(args)...));
    };
}

/**
 * @brief Designates objects fed to a hatching stage. Such objects can produce
 * multiple outputs as a result of being processed.
 *
 * @details A hatching stage is one that processes N number of Hatchable objects
 * and produces >=N output objects. This implies that after its first
 * invocation, a hatching stage is able to be invoked with empty(=false)
 * arguments. When a hatching stage produces an object contextually convertible
 * to false (e.g. empty optional or Hatchable), nullary invocations stop.
 *
 * @tparam T Type of the stored data.
 */
template <class T> struct Hatchable
{
    std::optional<T> data;

    Hatchable() = default;
    explicit Hatchable(T &&data) : data(std::move(data))
    {
    }
    explicit Hatchable(std::optional<T> &&data) : data(std::move(data))
    {
    }

    constexpr explicit operator bool() const noexcept
    {
        return data.has_value();
    }
};

template <class F> auto OutputHatchable(F &&operation)
{
    return [op = std::forward<F>(operation)](auto &&...args) mutable {
        return Hatchable(
            std::invoke(op, std::forward<decltype(args)>(args)...));
    };
}

} // namespace yap
