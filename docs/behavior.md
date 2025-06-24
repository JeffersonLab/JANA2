
# Behavior specification

## Introduction

## Execution Engine

### Engine initialization

If no `JEventSources` are present in the processing topology, `JApplication::Initialize()` will still succeed, and all present components will be initialized.

### Engine operation

If no `JEventSources` are present in the processing topology, `JExecutionEngine::Run()` will immediately throw a detailed `JException`. This will terminate `JApplication::Run()` and cause `JMain::Execute()` to exit the program.

### Factory auto activation
The `JAutoActivator` plugin, when enabled by setting the `autoactivate` parameter, always runs before the other `PhysicsEvent`-level JEventProcessors`. `JAutoActivator` calls the corresponding `JFactory::Create` for each data bundle in the `autoactivate` list, in the order provided.

### Max in-flight events
The number of in-flight events is controlled by the `jana:max_inflight_events` parameter, and defaults to the same value as the `nthreads` parameter. JANA2 will create this many `JEvents` in the pool at each `JEventLevel`. Increasing the number of in-flight events means more available tasks and hence better utilization of the worker threads, at the expense of more memory usage and longer startup time.

### Timeout
The execution engine will optionally enforce a timeout on arrow execution. If the timeout is exceeded, the supervisor will throw an exception, which will end all processing. The timeout duration is controlled by the `jana:timeout` and `jana:warmup_timeout` parameters. The warmup timeout exists because factories' `BeginRun` callback might take a long time when the event is used for the first time, for instance connecting to external resources such as calibration databases. The execution engine will decide whether to enforce the warmup timeout vs the general timeout by checking the `JEvent::IsWarmedUp` flag. This flag is initially set to false and is set to true once it has been processed successfully exactly once, as determined via `JEvent::Finish`.




