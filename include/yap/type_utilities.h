// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "buffer_queue.h"
#include "stage.h"

#include <future>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace yap
{

namespace detail
{

template <template <class, class> class Pair, class...> struct paired_list_impl;

template <template <class, class> class Pair, class... Args, std::size_t... Is>
struct paired_list_impl<Pair, std::tuple<Args...>, std::index_sequence<Is...>>
{
    using full_tuple_t = std::tuple<Args...>;

    using type =
        std::tuple<Pair<std::tuple_element_t<2 * Is, full_tuple_t>,
                        std::tuple_element_t<2 * Is + 1, full_tuple_t>>...>;
};

template <template <class, class> class Pair, class... Args> struct paired_list
{
    static_assert(sizeof...(Args) % 2 == 0,
                  "Only even typelists can be split into stages");

    using type = typename paired_list_impl<
        Pair, std::tuple<Args...>,
        std::make_index_sequence<sizeof...(Args) / 2>>::type;
};

template <class T1, class T2> using StageUptr = std::unique_ptr<Stage<T1, T2>>;

} // namespace detail

/**
 * @brief Executes the following transformation
 *
 * T1,T2,T3,T4 -> tuple<unique_ptr<Stage<T1,T2>>, unique_ptr<Stage<T3,T4>>>
 *
 * for even input type-lists.
 *
 * @tparam Args The type-list to transform.
 */
template <class... Args>
using stage_list_t =
    typename detail::paired_list<detail::StageUptr, Args...>::type;

namespace detail
{

template <class...> struct buffer_list_impl;

template <class... Args, size_t... Is>
struct buffer_list_impl<std::tuple<Args...>, std::index_sequence<Is...>>
{
    using full_tuple_t = std::tuple<Args...>;

    using type = std::tuple<std::shared_ptr<BufferQueue<
        std::future<std::tuple_element_t<2 * Is + 1, full_tuple_t>>>>...>;
};

template <class... Args> struct buffer_list
{
    static_assert(sizeof...(Args) % 2 == 0,
                  "Only even typelists can form a buffer list");
    using type = typename buffer_list_impl<
        std::tuple<Args...>,
        std::make_index_sequence<sizeof...(Args) / 2 - 1>>::type;
};

template <> struct buffer_list<>
{
    using type = std::tuple<>;
};

} // namespace detail

/**
 * @brief Executes the following transformation
 *
 * T0,T1,T1,T2,T2,T3 -> tuple<shared_ptr<BufferQueue<T1>>,
 * shared_ptr<BufferQueue<T2>>>
 *
 * for even input type-lists.
 *
 * @tparam Args The type-list to transform.
 */
template <class... Args>
using buffer_list_t = typename detail::buffer_list<Args...>::type;

namespace detail
{

template <class> struct last_type_impl;

template <> struct last_type_impl<std::tuple<>>
{
};

template <class... Args> struct last_type_impl<std::tuple<Args...>>
{
    using tuple_t = std::tuple<Args...>;
    using type = std::tuple_element_t<std::tuple_size_v<tuple_t> - 1, tuple_t>;
};

} // namespace detail

template <class... Args>
using last_type = detail::last_type_impl<std::tuple<Args...>>;

template <class... Args> using last_type_t = typename last_type<Args...>::type;

} // namespace yap
