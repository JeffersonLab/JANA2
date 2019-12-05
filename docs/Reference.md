
---
title: JANA: Multi-threaded HENP Event Reconstruction
---

<center>
<table border="0" width="100%" align="center">
<TH width="20%"><A href="index.html">Welcome</A></TH>
<TH width="20%"><A href="Tutorial.html">Tutorial</A></TH>
<TH width="20%"><A href="Howto.html">How-to guides</A></TH>
<TH width="20%"><A href="Explanation.html">Principles</A></TH>
<TH width="20%"><A href="Reference.html">Reference</A></TH>
</table>
</center>

Reference
=========
This section describes the underlying machinery of JANA. This is essentially a launchpad into Doxygen.

## Main API

* [JApplication](html/class_j_application.html): The main entry point into the program
* [JObject](html/class_j_object.html): Data containers for specific results
* [JEventSource](html/class_j_event_source.html): From a file or messaging producer, expose a stream of events. Each event is an aggregate of JObjects
* [JFactory](html/class_j_factory.html): Given an event, calculate a specific result, potentially calling other JFactories recursively to obtain any prereqs
* [JEventProcessor](html/class_j_event_processor.html): Run desired JFactories over the event stream, writing results to an output file or messaging consumer

## Streaming Extensions

* [JStreamingEventSource](html/class_j_streaming_event_source.html): A starting point for structured, composable streaming
* [JTransport](html/class_j_transport.html): An interface for a generic messaging transport
* [JMessage](html/struct_j_message.html): An interface for a stream buffer

## User-facing utilities

* [JParameter](html/class_j_parameter.html): Configure the behavior of components at runtime
* [JLogger](html/struct_j_logger.html): Publish debugging information to standard out in a structured, convenient, threadsafe way
* [JService](html/struct_j_service.html): Share external (stateful) services such as calibration constants and magnetic field maps
* [JCsvWriter](html/class_j_csv_writer.html): Conveniently debug a JFactory by writing its generated JObjects to CSV

## Internal services

* [JLoggingService](html/class_j_logging_service.html): Furnish the user with a logger already configured for that particular component
* [JParameterManager](html/class_j_parameter_manager.html): Furnish the user with parameters extracted from command line flags and configuration files

## Parallelism engine

* [JProcessingController](html/class_j_processing_controller.html): The interface which any parallelism engine must adhere to
* [JArrowProcessingController](html/class_j_arrow_processing_controller.html): The entry point into the "Arrow" engine
* [JWorker](html/class_j_worker.html): Contains the loop for each worker thread, along with startup/shutdown logic and encapsulated worker state.
* [JScheduler](html/class_j_scheduler.html): Contains the logic for giving a worker a new assignment


