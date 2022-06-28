## Basic pipeline usage examples

### nthreaded_stages

Showcase the usage of stages that act in a multi-threaded fashion internally. This means that not only each stage runs in its own thread, 
but the operation of the stage is processed in parallel and a future containing the result of  the operation is returned. To emulate the 
existence of an "internal thread pool", stages return futures to the results. The pipeline is able to move the futures around and maintain 
the FIFO property of the data stream.

In this example, the generator stage will asynchronously wait for 500ms. This means that after e.g. 1s of running the pipeline, thousands 
of generation steps will have finished since the 500ms waiting time is not "spent sequentially". Those outputs will be ready for console 
output in the sink stage, since the "input buffer" for that stage will have collected all tasks awaiting to finish by then. This behavior 
manifests as a 500ms hiccup after calling run, followed by continuously printing the output in FIFO order.

### use_non_copyable_type

A non copyable type flowing through the pipeline. This demo shows how yap leverages move semantics to progress data from one stage to the next.
