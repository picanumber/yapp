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
};

namespace detail
{

template <class> constexpr bool kIsFiltered = false;
template <class T> constexpr bool kIsFiltered<Filtered<T>> = true;

} // namespace detail

template <class F> auto Filter(F &&operation)
{
    return [op = std::forward<F>(operation)](auto &&...args) {
        return Filtered{std::invoke(op, std::forward<decltype(args)>(args)...)};
    };
}

} // namespace yap
