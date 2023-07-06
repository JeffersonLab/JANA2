Reference
========

This section describes the underlying machinery of JANA. This is essentially a launchpad into Doxygen.

Main API
-----------

*`JApplication <https://www.jlab.org/JANA/jana_doc_latest/class_j_application.html>`: The main entry point into the program
*`JObject <https://www.jlab.org/JANA/jana_doc_latest/class_j_object.html`: Data containers for specific results
*`JEventSource <https://www.jlab.org/JANA/jana_doc_latest/class_j_event_source.html>` : From a file or messaging producer, expose a stream of events. Each event is an aggregate of JObjects
*`JFactory <https://www.jlab.org/JANA/jana_doc_latest/class_j_factory.html>` : Given an event, calculate a specific result, potentially calling other JFactories recursively to obtain any prereqs
*`JEventProcessor <https://www.jlab.org/JANA/jana_doc_latest/class_j_event_processor.html`: Run desired JFactories over the event stream, writing results to an output file or messaging consumer

Streaming Extensions
----------------------

*JStreamingEventSource: A starting point for structured, composable streaming
*JTransport: An interface for a generic messaging transport
*JMessage: An interface for a stream buffer

User-facing utilities
-----------------------

*JParameter: Configure the behavior of components at runtime
*JLogger: Publish debugging information to standard out in a structured, convenient, threadsafe way
*JService: Share external (stateful) services such as calibration constants and magnetic field maps
*JCsvWriter: Conveniently debug a JFactory by writing its generated JObjects to CSV

Internal services
------------------------

*JLoggingService: Furnish the user with a logger already configured for that particular component
*JParameterManager: Furnish the user with parameters extracted from command line flags and configuration files

Parallelism engine
----------------------

*JProcessingController: The interface which any parallelism engine must adhere to
*JArrowProcessingController: The entry point into the “Arrow” engine
*JWorker: Contains the loop for each worker thread, along with startup/shutdown logic and encapsulated worker state.
*JScheduler: Contains the logic for giving a worker a new assignment
