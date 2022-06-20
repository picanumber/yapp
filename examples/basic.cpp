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

class NoCopy
{
    int _id;
    static int classId;

  public:
    NoCopy() : _id(classId++)
    {
    }

    NoCopy(NoCopy const &) = delete;
    NoCopy &operator=(NoCopy const &) = delete;

    NoCopy(NoCopy &&) = default;
};

int NoCopy::classId = 0;

int main()
{
    static auto generator = [val = 1]() mutable {
        std::cout << "Generating..\n";
        // std::this_thread::sleep_for(500ms);
        return NoCopy{};
    };
    static auto transform = [](NoCopy arg) {
        std::cout << "Transform1\n";
        // std::this_thread::sleep_for(200ms);
        return arg;
    };

    std::vector<NoCopy> output;
    static auto sink = [&output](NoCopy arg) {
        std::cout << "Output...\n";
        // std::this_thread::sleep_for(500ms);
        output.emplace_back(std::move(arg));
    };

    yap::Pipeline pipe0;
    yap::Pipeline pipe1(std::move(pipe0), generator);

    yap::Pipeline pipe2(std::move(pipe1), transform);
    yap::Pipeline pipe3(std::move(pipe2), sink);
    pipe3.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    pipe3.stop();
    std::cout << "Stopping\n";

    return 0;
}

