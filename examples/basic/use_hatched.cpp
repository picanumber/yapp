#include "yap/pipeline.h"
#include "yap/runtime_utilities.h"
#include "yap/topology.h"

#include <chrono>
#include <iostream>
#include <optional>

using namespace std::chrono_literals;

int main()
{
    auto generator = [val = 0]() mutable {
        if (val > 9)
        {
            throw yap::GeneratorExit{};
        }
        return val++;
    };

    auto hatchingTransform = [curVal = 0,
                              curChar = 'a'](yap::Hatched<int> val) mutable {
        std::optional<char> ret;

        if (val)
        {
            curChar = 'a' + *val.data;
            curVal = *val.data;
            ret.emplace(curChar);
        }
        else if (curVal-- > 0)
        {
            ret.emplace(curChar);
        }

        return ret;
    };

    auto sink = [](std::optional<char> val) {
        std::cout << "Output: " << val.value() << std::endl;
    };

    auto pl = yap::Pipeline{} | yap::Hatch(std::move(generator)) |
              hatchingTransform | sink;

    std::cout << "Processing\n";
    pl.consume();
    std::cout << "Finished\n";

    return 0;
}
