#include "yap/pipeline.h"

#include <chrono>
#include <iostream>
#include <optional>

using namespace std::chrono_literals;

int main()
{
    auto generator = [val = 0]() mutable { return val++; };
    auto transform = [](int val) {
        std::optional<int> ret;
        if (val % 2)
        {
            ret.emplace(val);
        }

        std::this_thread::sleep_for(100ms);
        return ret;
    };

    auto sink = [](yap::Filtered<int> val) {
        std::cout << "Output: " << val.data.value() << std::endl;
    };

    auto pl =
        yap::Pipeline{} | generator | yap::Filter(std::move(transform)) | sink;

    std::cout << "Run\n";
    pl.run();
    std::this_thread::sleep_for(5s);
    pl.stop();
    std::cout << "Stopped\n";

    return 0;
}
