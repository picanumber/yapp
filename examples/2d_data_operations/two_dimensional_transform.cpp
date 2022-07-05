#include "yap/pipeline.h"

#include <algorithm>
#include <bits/c++config.h>
#include <chrono>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{

template <typename T> struct random_generator
{
    enum
    {
        is_signed = std::is_signed<T>::value
    };
    using distribution_t =
        std::conditional_t<std::is_integral<T>::value,
                           std::uniform_int_distribution<T>,
                           std::uniform_real_distribution<T>>;

    using result_type = typename distribution_t::result_type;

    std::random_device rd;
    std::mt19937 gen;
    distribution_t dis;

    explicit random_generator(T width, bool steady = true)
        : rd{}, gen{steady ? 1 : rd()}, dis{is_signed ? -(width / 2.) : 0,
                                            is_signed ? (width / 2.) : width}
    {
    }

    random_generator(T low, T high, bool steady = true)
        : rd{}, gen{steady ? 1 : rd()}, dis{low, high}
    {
    }

    result_type operator()()
    {
        return dis(gen);
    }

    random_generator(random_generator<T> const &) = delete;
    random_generator(random_generator<T> &&) = delete;
    random_generator &operator=(random_generator<T> const &) = delete;
};

class MatGenerator
{
    std::shared_ptr<random_generator<unsigned>> _rgen;
    std::size_t _width, _height;
    std::size_t _reps = 0;

    std::vector<std::vector<int>> _lastMat;

  public:
    MatGenerator(std::size_t width, std::size_t height)
        : _rgen(std::make_shared<random_generator<unsigned>>(0, 255, true)),
          _width(width), _height(height)
    {
        _lastMat = make();
    }

    std::vector<std::vector<int>> operator()()
    {
        if (++_reps > 10'000)
        {
            throw yap::GeneratorExit{};
        }

        if (_reps % 5 == 0)
        {
            _lastMat = make();
            return _lastMat;
        }
        else
        {
            return _lastMat;
        }
    }

  private:
    std::vector<std::vector<int>> make()
    {
        std::vector<std::vector<int>> v(_height, std::vector<int>(_width));

        for (std::size_t i(0); i < _height; ++i)
        {
            for (std::size_t j(0); j < _width; ++j)
            {
                v[i][j] = (*_rgen)();
            }
        }

        return v;
    }
};

struct MatNormalizer
{
    std::vector<std::vector<float>> operator()(
        std::vector<std::vector<int>> arg)
    {
        std::vector<std::vector<float>> ret(
            arg.size(), std::vector<float>(arg.front().size()));

        float sum{0};
        for (std::size_t i(0); i < arg.size(); ++i)
        {
            for (std::size_t j(0); j < arg.front().size(); ++j)
            {
                sum += arg[i][j];
                ret[i][j] = arg[i][j];
            }
        }

        for (auto &v : ret)
            for (float &elem : v)
                elem /= sum;

        return ret;
    }
};

struct MatModifier
{
    std::vector<std::vector<float>> operator()(
        std::vector<std::vector<float>> arg)
    {
        for (std::size_t i(0); i < arg.size(); ++i)
        {
            for (std::size_t j(0); j < arg.front().size(); ++j)
            {
                auto up = i > 0 ? arg[i - 1][j] : 0;
                auto down = i < arg.size() - 1 ? arg[i + 1][j] : 0;
                auto left = j > 0 ? arg[i][j - 1] : 0;
                auto right = j < arg.front().size() - 1 ? arg[i][j + 1] : 0;

                arg[i][j] += up + down + left + right;
            }
        }

        return arg;
    }
};

class MatCoefWriter
{
    float &result;

  public:
    explicit MatCoefWriter(float &out) : result(out)
    {
    }

    void operator()(std::vector<std::vector<float>> arg)
    {
        float sum{};
        for (std::size_t i(0); i < arg.size(); ++i)
        {
            for (std::size_t j(0); j < arg.front().size(); ++j)
            {
                sum += arg[i][j];
            }
        }

        for (std::size_t i(0); i < arg.size(); ++i)
        {
            for (std::size_t j(0); j < arg.front().size(); ++j)
            {
                sum -= arg[i][j];
            }
        }

        for (std::size_t i(0); i < arg.size(); ++i)
        {
            for (std::size_t j(0); j < arg.front().size(); ++j)
            {
                sum += arg[i][j];
            }
        }

        result += sum;
    }
};

} // namespace

int main(int, char *[])
{
    float out;
    auto pl = yap::Pipeline{} | MatGenerator(255, 255) | MatNormalizer{} |
              MatModifier{} | MatCoefWriter(out);

    pl.consume();

    return 0;
} // pl will stop on its destructor.
