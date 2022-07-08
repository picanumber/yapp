// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

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

} // namespace yap
