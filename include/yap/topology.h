// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "runtime_utilities.h"

#include <functional>
#include <optional>

namespace yap
{

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
    return [op = std::forward<F>(operation)](auto &&...args) {
        return Filtered(std::invoke(op, std::forward<decltype(args)>(args)...));
    };
}

} // namespace yap
