#include "yap/pipeline.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

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

    auto pl = yap::Pipeline{} | generator | transform | sink;
    pl.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::cout << "Pausing\n";
    pl.pause();
    std::cout << "Paused\n";

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Resuming\n";
    pl.run();

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    std::cout << "Stopping\n";
    pl.stop();
    std::cout << "Stopped\n";

    return 0;
}
