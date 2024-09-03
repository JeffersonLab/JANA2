JANA2 C++ Reference Guide
=========================

JANA2 is a cutting-edge C++ framework designed for High Energy and Nuclear Physics experimental data reconstruction. It excels in multi-threaded processing, ensuring maximum efficiency and scalability. With its intuitive setup and extensive customization options, JANA2 empowers both novice and experienced programmers to contribute effectively. Whether running on a local machine or a large computing cluster, JANA2 optimizes performance, making it the ideal choice for any scale of project. 

- Read more information on [MAIN DOCUMENTATION](https://JeffersonLab.github.io/JANA2/) website
- Contribute [on GitHub](https://github.com/JeffersonLab/JANA2)

This website provides documentation for JANA2 C++ API automatically generated by Doxygen. 

## Main API

* [JApplication](class_j_application.html): The main entry point into the program
* [JObject](class_j_object.html): Data containers for specific results
* [JEventSource](class_j_event_source.html): From a file or messaging producer, expose a stream of events. Each event is an aggregate of JObjects
* [JFactory](class_j_factory.html): Given an event, calculate a specific result, potentially calling other JFactories recursively to obtain any prereqs
* [JEventProcessor](class_j_event_processor.html): Run desired JFactories over the event stream, writing results to an output file or messaging consumer

## Streaming Extensions

* [JStreamingEventSource](class_j_streaming_event_source.html): A starting point for structured, composable streaming
* [JTransport](struct_j_transport.html): An interface for a generic messaging transport
* [JMessage](struct_j_message.html): An interface for a stream buffer

## User-facing utilities

* [JParameter](class_j_parameter.html): Configure the behavior of components at runtime
* [JLogger](struct_j_logger.html): Publish debugging information to standard out in a structured, convenient, threadsafe way
* [JService](struct_j_service.html): Share external (stateful) services such as calibration constants and magnetic field maps

## Internal services

* [JLoggingService](class_j_logging_service.html): Furnish the user with a logger already configured for that particular component
* [JParameterManager](class_j_parameter_manager.html): Furnish the user with parameters extracted from command line flags and configuration files

## Parallelism engine

* [JProcessingController](class_j_processing_controller.html): The interface which any parallelism engine must adhere to
* [JArrowProcessingController](class_j_arrow_processing_controller.html): The entry point into the "Arrow" engine
* [JWorker](class_j_worker.html): Contains the loop for each worker thread, along with startup/shutdown logic and encapsulated worker state.
* [JScheduler](class_j_scheduler.html): Contains the logic for giving a worker a new assignment

