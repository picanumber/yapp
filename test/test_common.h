// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include <chrono>
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

} // namespace tcn
