[![Ubuntu](https://github.com/picanumber/yap_prelude/actions/workflows/ubuntu.yml/badge.svg)](https://github.com/picanumber/yap_prelude/actions/workflows/ubuntu.yml) [![Codecov](https://github.com/picanumber/yap_prelude/actions/workflows/codecov.yml/badge.svg)](https://github.com/picanumber/yap_prelude/actions/workflows/codecov.yml) [![Style](https://github.com/picanumber/yap_prelude/actions/workflows/style.yml/badge.svg)](https://github.com/picanumber/yap_prelude/actions/workflows/style.yml) [![Memory](https://github.com/picanumber/yap_prelude/actions/workflows/asan.yml/badge.svg)](https://github.com/picanumber/yap_prelude/actions/workflows/asan.yml) [![Threading](https://github.com/picanumber/yap_prelude/actions/workflows/tsan.yml/badge.svg)](https://github.com/picanumber/yap_prelude/actions/workflows/tsan.yml)

![pipeline design](assets/pipeline_diagram2.png)

# Contents
1. [Design](##Design)
2. [Motivation](##Motivation)
3. [Construction](##Construction)
4. [Operations](##Operations)
5. [Examples](##Examples)
6. [Installation](##Installation)

## Design

yap stands for __yet another pipeline__. It is a zero dependency, header only library providing a multi-threaded implementation of the pipeline pattern. This enables a user to define a series of transformations where:

1. Each stage runs in its own thread.
2. Buffering exists between stages to regulate stages of different latency.
3. The first stage is considered a __generator__, meaning it's the entity that feeds data into the pipeline.
4. Intermediate stages can have any `input`/`output` type, provided that the resulting chain is feasible, e.g. output of stage 2 can be an input to stage 3. Correctness in this respect is checked during compilation time.
5. The final stage is considered a __sink__, meaning it's the entity that extracts data from the pipeline.

## Motivation

* Zero dependencies
* Vanilla c++, depends on C++20 alone
* Using C++ standard `<thread>` library
* why pipeline at all -> ...

## Construction

This section outlines how to create a pipeline. To help drive our points, assume the existence of the following callables:

```cpp
auto generator = [val = 0] { return val++; };
auto transform = [](int val) { return std::to_string(val); };
auto sink = [](std::string const& s) { std::cout << s << std::endl; };
```

### Strongly typed

To construct a pipeline with a type mandated by the input/output types of each stage, simply pipe the required stages into `yap::Pipeline{}`:

```cpp
auto ps = yap::Pipeline{} | generator | transform | sink;
// ps.run() etc can be called on the object.
```

The `pipeLine` object above will be typed as `yap::Pipeline<void,int,int,string,string,void>`. A user does not have to specify the type that results from the specified transformations, since [CTAD](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction) handles this process. A strongly typed pipeline can be moved into a "greater" pipeline to be chained with additional stages at a later point, provided that a sink stage has not been attached.

### Polymorphic

In cases where having a strong type is cumbersome or plainly does not offer much benefit, e.g. when no special treatment is planned for pipeline objects of different types or when a pipeline member object needs to create as less noise to the containing class, the user can choose to use a pipeline through its polymorhic base class:

```cpp
auto pp = yap::make_pipeline(generator, transform, sink);
// pp->run() etc can be called on the object.
```

The object returned from the `make_pipeline` function, is a `unique_ptr<yap::pipeline>`. This lower-case `pipeline` class, is the abstract definition of a pipeline and even though information on type transformations is lost, all operations are carried out in way consistent to its construction properties. A polymorphic pipeline cannot be chained further, since information on how to relay types is lost.

## Operations

### Run

### Stop

### Pause

### Consume

## Examples

## Installation

mention building is needed only for the testing suite.
