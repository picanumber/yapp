// © 2022 Nikolaos Athanasiou, github.com/picanumber
#pragma once

#include "buffer_queue.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace yap
{

namespace detail
{

template <class IN, class OUT> struct Function
{
    using type = std::function<OUT(IN)>;
};
template <class OUT> struct Function<void, OUT>
{
    using type = std::function<OUT()>;
};
template <class IN, class OUT>
using function_t = typename Function<IN, OUT>::type;

// Process a transformation stage.
template <class IN, class OUT>
void process(std::function<OUT(IN)> &op, BufferQueue<IN> *input,
             BufferQueue<OUT> *output)
{
    try
    {
        // expect not null input/output
        output->push(op(input->pop()));
    }
    catch (std::out_of_range &e)
    {
        // The input queue doesn't want me to wait.
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
    }
}

// Process a generator stage.
template <class OUT>
void process(std::function<OUT()> &op, BufferQueue<void> * /*input*/,
             BufferQueue<OUT> *output)
{
    try
    {
        // expect null input, non null output.
        output->push(op());
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
    }
}

// Process a sink stage.
template <class IN>
void process(std::function<void(IN)> &op, BufferQueue<IN> *input,
             BufferQueue<void> * /*output*/)
{
    try
    {
        // expect null output, non null input.
        op(input->pop());
    }
    catch (std::out_of_range &e)
    {
        // The input queue doesn't want me to wait.
    }
    catch (...)
    {
        // Op threw an exception. No point in propagating the data.
    }
}

} // namespace detail

template <class IN, class OUT> class Stage
{
    detail::function_t<IN, OUT> _operation;
    BufferQueue<IN> *_input = nullptr;
    BufferQueue<OUT> *_output = nullptr;
    std::thread _worker;
    std::mutex _cmdMtx;
    std::atomic_bool _alive{false};

  public:
    Stage() = default; // TODO: Remove this!

    template <class F>
    Stage(F &&operation) : _operation(std::forward<F>(operation))
    {
    }

    ~Stage()
    {
        stop();
    }

    Stage(Stage &&other)
        : _operation(std::move(other._operation)), _input(other._input),
          _output(other._output)
    {
        other._input = nullptr;
        other._output = nullptr;
    }

    void start(BufferQueue<IN> *input, BufferQueue<OUT> *output)
    {
        std::lock_guard lk(_cmdMtx);
        if (!_alive)
        {
            _input = input;
            _output = output;

            _alive = true;
            _worker = std::thread(&Stage::process, this);
        }
    }

    void stop()
    {
        std::lock_guard lk(_cmdMtx);
        if (_alive)
        {
            _alive = false;
            _worker.join();
        }
    }

  private:
    void process()
    {
        while (_alive)
        {
            detail::process(_operation, _input, _output);
        }
    }
};

} // namespace yap
