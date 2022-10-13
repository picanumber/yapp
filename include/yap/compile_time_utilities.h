// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include <type_traits>

namespace yap
{

namespace detail
{

template <class T, template <class...> class Template>
struct is_instantiation : std::false_type
{
};

template <template <class...> class Template, class... Args>
struct is_instantiation<Template<Args...>, Template> : std::true_type
{
};

} // namespace detail

template <class T, template <class...> class G>
concept instantiation_of = detail::is_instantiation<T, G>::value;

} // namespace yap
