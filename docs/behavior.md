
# Behavior specification

## Introduction

## Initialization
If no `JEventSources` are present in the processing topology, `JApplication::Initialize()` will still succeed, and all present components will be initialized.

## Execution
If no `JEventSources` are present in the processing topology, `JExecutionEngine::Run()` will immediately throw a detailed `JException`. This will terminate `JApplication::Run()` and cause `JMain::Execute()` to exit the program.

The `JAutoActivator` plugin, when enabled by setting the `autoactivate` parameter, always runs before the other `PhysicsEvent`-level JEventProcessors`. `JAutoActivator` calls the corresponding `JFactory::Create` for each data bundle in the `autoactivate` list, in the order provided.

The number of in-flight events is controlled by the `jana:max_inflight_events` parameter, and defaults to the same value as the `nthreads` parameter. JANA2 will create this many `JEvents` in the pool at each `JEventLevel`. Increasing the number of in-flight events means more available tasks and hence better utilization of the worker threads, at the expense of more memory usage and longer startup time.


## Finalization



