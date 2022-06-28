#include "yap/pipeline.h"

#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

/**
 * Showcase the usage of stages that act in a multi-threaded fashion internally.
 * This means that not only each stage runs in each own thread, but the
 * operation of the stage is processed in parallel and a future containing the
 * result of the operation is returned. To emulate the existence of an "internal
 * thread pool", stages return futures to the results. The pipeline is able to
 * move the futures around and maintain the FIFO property of the data stream.
 *
 * In this example, the generator stage will asynchronously wait for 500ms. This
 * means that after e.g. 1s of running the pipeline, thousands of generation
 * steps will have finished since the 500ms waiting time is not "spent
 * sequentially". Those outputs will be ready for console output in the sink
 * stage, since the "input buffer" for that stage will have collected all tasks
 * awaiting to finish by then. This behavior manifests as a 500ms hiccup after
 * calling run, followed by continuously printing the output in FIFO order.
 */
int main()
{
    static auto generator = [val = 1]() mutable {
        ++val;
        return std::async([val] {
            std::this_thread::sleep_for(500ms);
            return val;
        });
    };

    static auto t1 = [](std::future<int> arg) {
        return std::async([a = std::move(arg)]() mutable { return a.get(); });
    };

    static auto t2 = [](std::future<int> arg) {
        return std::async([a = std::move(arg)]() mutable { return a.get(); });
    };

    auto sink = [](std::future<int> arg) {
        std::cout << arg.get() << std::endl;
    };

    auto pl = yap::Pipeline{} | generator | t1 | t2 | sink;
    std::cout << "Run\n";
    pl.run();

    std::this_thread::sleep_for(5s);

    pl.stop();
    std::cout << "Stopped\n";

    return 0;
}
