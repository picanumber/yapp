// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include <chrono>
#if __cpp_concepts
#include <concepts>
#endif
#include <utility>

using namespace std::chrono_literals;

namespace tcn
{

[[maybe_unused]] constexpr std::size_t kSmallInputSz{100};
[[maybe_unused]] constexpr std::size_t kMidInputSz{10'000};
[[maybe_unused]] constexpr std::size_t kLargeInputSz{1'000'000};

template <class T> class MoveOnly
{
    T _value;

  public:
    explicit MoveOnly(T value) : _value(value)
    {
    }

    MoveOnly(MoveOnly const &) = delete;
    MoveOnly &operator=(MoveOnly const &) = delete;

    MoveOnly(MoveOnly &&other) : _value(std::move(other._value))
    {
    }

    T get() const
    {
        return _value;
    }
};

#if __cpp_concepts
template <std::integral I> class Iota
#else
template <class I> class Iota
#endif
{
    I _val;

  public:
    explicit Iota(I val) : _val(val)
    {
    }

    I operator()()
    {
        return _val++;
    }
};

#if __cpp_concepts
bool is_humble(std::integral auto i)
#else
bool is_humble(auto i)
#endif
{
    if (i <= 1)
    {
        return true;
    }
    if (i % 2 == 0)
    {
        return is_humble(i / 2);
    }
    if (i % 3 == 0)
    {
        return is_humble(i / 3);
    }
    if (i % 5 == 0)
    {
        return is_humble(i / 5);
    }
    if (i % 7 == 0)
    {
        return is_humble(i / 7);
    }
    return false;
}

} // namespace tcn
