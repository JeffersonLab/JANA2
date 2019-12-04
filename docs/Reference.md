Reference
=========
This section describes the underlying machinery of JANA. This is essentially a launchpad into Doxygen.

## API Reference

The key components of the API are:

* JApplication: The main entry point into the program
* JObject: Data containers for specific results
* JEventSource: From a file or messaging producer, expose a stream of events. Each event is an aggregate of JObjects
* JFactory: Given an event, calculate a specific result, potentially calling other JFactories recursively to obtain any prereqs
* JEventProcessor: Run desired JFactories over the event stream, writing results to an output file or messaging consumer

## User-facing utilities

* JLogger: Publish debugging information to standard out in a structured, convenient, threadsafe way
* JService: Share external (stateful) services such as calibration constants and magnetic field maps
* JParameter: Configure the behavior of components at runtime

## Internal services

* JLoggingService: Furnish the user with a logger already configured for that particular component
* JParameterManager: Furnish the user with parameters extracted from command line flags and configuration files
* 


## Parallelism engine

* JProcessingController: The interface which any parallelism engine must adhere to
* JArrowProcessingController: The entry point into the "Arrow" engine
* JWorker: Contains the loop for each worker thread, along with startup and shutdown logic and encapsulated worker state.
* JScheduler: Contains the logic for giving a worker a new assignment


## Streaming

* 
