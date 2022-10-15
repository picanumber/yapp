#include "yap/pipeline.h"
#include "yap/runtime_utilities.h"
#include "yap/topology.h"

#include <chrono>
#include <iostream>
#include <optional>

using namespace std::chrono_literals;

int main()
{
    std::vector<int> input{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};

    // Accepts Hatchable values, which means each input can produce multiple
    // outputs. Such a production happens in a "piecewise" manner meaning that
    // the stage does not return a collection of values; instead every piece of
    // output is immediately pushed to the next stage and the stage is invoked
    // again to produce the rest of the output.
    auto hatchingTransform = [curVal = 0,
                              curChar = 'a'](yap::Hatchable<int> val) mutable {
        std::optional<char> ret;

        if (val)
        {
            // New Input from previous stage.
            curChar = 'a' + *val.data;
            curVal = *val.data;
            ret.emplace(curChar);
        }
        else if (curVal-- > 0)
        {
            // Keep processing the last input from previous stage.
            ret.emplace(curChar);
        }

        return ret; // Returning a contextually "false" object, here empty
                    // optional, means the input won't be hatched any more and
                    // the stage can process new values produced from the
                    // previous stage.
    };

    auto sink = [](std::optional<char> val) {
        std::cout << "Output: " << val.value() << std::endl;
    };

    auto pl = yap::Pipeline{} |
              yap::OutputHatchable(yap::Consume(input.begin(), input.end())) |
              hatchingTransform | sink;

    std::cout << "Processing\n";
    pl.consume();
    std::cout << "Finished\n";

    return 0;
}
