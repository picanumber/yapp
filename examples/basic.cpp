#include "yap/pipeline.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

template <class F> void foo(F)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
}

int main()
{
    static auto generator = [val = 1]() mutable {
        std::cout << "Generating..\n";
        // std::this_thread::sleep_for(500ms);
        return val++;
    };
    static auto transform = [](int arg) {
        std::cout << "Transform1\n";
        // std::this_thread::sleep_for(200ms);
        return arg * 2.;
    };

    std::vector<double> output;
    static auto sink = [&output](double arg) {
        std::cout << "Output...\n";
        // std::this_thread::sleep_for(500ms);
        output.push_back(arg);
    };

    yap::Pipeline pipe0;
    yap::Pipeline pipe1(std::move(pipe0), generator);

    yap::Pipeline pipe2(std::move(pipe1), transform);
    yap::Pipeline pipe3(std::move(pipe2), sink);
    pipe3.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    return 0;
    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        std::cout << "Half a second elapsed ........ \n";
    }

    return 0;
}

