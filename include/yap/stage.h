// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "buffer_queue.h"
#include "compile_time_utilities.h"
#include "runtime_utilities.h"
#include "topology.h"

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <type_traits>

namespace yap
{

namespace detail
{

// Process a transformation stage. Returns whether to keep processing.
template <class IN, class OUT>
bool process(Callable<IN, OUT> &op,
             std::shared_ptr<BufferQueue<std::future<IN>>> &input,
             std::shared_ptr<BufferQueue<std::future<OUT>>> &output)
{
    try
    {
        // expect not null input/output
        output->push(make_ready_future<OUT>(op(input->pop().get())));
        return true;
    }
    catch (detail::ClosedError &e)
    {
        return false;
    }
    catch (GeneratorExit &e)
    {
        output->push(make_exceptional_future<OUT>(e));
        return false;
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
        return true;
    }
}

// Process a filtering transformation stage. Returns whether to keep processing.
template <class IN, class OUT>
    requires(instantiation_of<OUT, Filtered>) bool
process(Callable<IN, OUT> &op,
        std::shared_ptr<BufferQueue<std::future<IN>>> &input,
        std::shared_ptr<BufferQueue<std::future<OUT>>> &output)
{
    try
    {
        if (auto result = op(input->pop().get()); result.data)
        {
            output->push(make_ready_future<OUT>(std::move(result)));
        }
        return true;
    }
    catch (detail::ClosedError &e)
    {
        return false;
    }
    catch (GeneratorExit &e)
    {
        output->push(make_exceptional_future<OUT>(e));
        return false;
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
        return true;
    }
}

// Process a hatching transformation stage. Returns whether to keep processing.
template <class IN, class OUT>
    requires(instantiation_of<IN, Hatchable>) bool
process(Callable<IN, OUT> &op,
        std::shared_ptr<BufferQueue<std::future<IN>>> &input,
        std::shared_ptr<BufferQueue<std::future<OUT>>> &output)
{
    try
    {
        auto result = op(input->pop().get());
        while (result)
        {
            output->push(make_ready_future<OUT>(std::move(result)));
            result = op(IN{});
        }
        return true;
    }
    catch (detail::ClosedError &e)
    {
        return false;
    }
    catch (GeneratorExit &e)
    {
        output->push(make_exceptional_future<OUT>(e));
        return false;
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
        return true;
    }
}

// Process a generator stage.
template <class OUT>
bool process(Callable<void, OUT> &op,
             std::shared_ptr<BufferQueue<std::future<void>>> & /*input*/,
             std::shared_ptr<BufferQueue<std::future<OUT>>> &output)
{
    try
    {
        // expect null input, non null output.
        output->push(make_ready_future<OUT>(op()));
        return true;
    }
    catch (detail::ClosedError &e)
    {
        return false;
    }
    catch (GeneratorExit &e)
    {
        output->push(make_exceptional_future<OUT>(e));
        return false;
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
        return true;
    }
}

// Process a sink stage.
template <class IN>
bool process(Callable<IN, void> &op,
             std::shared_ptr<BufferQueue<std::future<IN>>> &input,
             std::shared_ptr<BufferQueue<std::future<void>>> &
             /*output*/)
{
    try
    {
        // expect null output, non null input.
        op(input->pop().get());
        return true;
    }
    catch (detail::ClosedError &e)
    {
        return false;
    }
    catch (GeneratorExit &e)
    {
        return false;
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
        return true;
    }
}

} // namespace detail

template <class IN, class OUT> class Stage
{
    Callable<IN, OUT> _operation;
    std::shared_ptr<BufferQueue<std::future<IN>>> _input;
    std::shared_ptr<BufferQueue<std::future<OUT>>> _output;
    std::thread _worker;
    std::mutex _cmdMtx;
    std::atomic_bool _alive{false};

  public:
    template <class F>
    explicit Stage(F &&operation) : _operation(std::forward<F>(operation))
    {
    }

    ~Stage()
    {
        stop();
    }

    void start(std::shared_ptr<BufferQueue<std::future<IN>>> input,
               std::shared_ptr<BufferQueue<std::future<OUT>>> output)
    {
        std::lock_guard lk(_cmdMtx);
        if (!_alive)
        {
            _input = std::move(input);
            _output = std::move(output);

            _alive = true;
            _worker = std::thread(&Stage::process, this);
        }
    }

    auto stop()
    {
        return std::async([this] {
            std::lock_guard lk(_cmdMtx);
            if (_alive)
            {
                _alive = false;
                _worker.join();
            }
        });
    }

    // Let the worker run, until it exits due to ClosedBuffer error.
    void consume()
    {
        std::lock_guard lk(_cmdMtx);
        if (_alive)
        {
            _worker.join();
            _alive = false;
        }
    }

  private:
    void process()
    {
        while (_alive)
        {
            if (!detail::process(_operation, _input, _output))
            {
                break;
            }
        }
    }
};

} // namespace yap
