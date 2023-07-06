Reference
========

This section describes the underlying machinery of JANA. This is essentially a launchpad into Doxygen.

Main API
-----------

* `JApplication <https://www.jlab.org/JANA/jana_doc_latest/class_j_application.html>`_: The main entry point into the program
* `JObject <https://www.jlab.org/JANA/jana_doc_latest/class_j_object.html>`_: Data containers for specific results
* `JEventSource <https://www.jlab.org/JANA/jana_doc_latest/class_j_event_source.html>`_ : From a file or messaging producer, expose a stream of events. Each event is an aggregate of JObjects
* `JFactory <https://www.jlab.org/JANA/jana_doc_latest/class_j_factory.html>`_ : Given an event, calculate a specific result, potentially calling other JFactories recursively to obtain any prereqs
* `JEventProcessor <https://www.jlab.org/JANA/jana_doc_latest/class_j_event_processor.html>`_: Run desired JFactories over the event stream, writing results to an output file or messaging consumer

Streaming Extensions
----------------------

* `JStreamingEventSource <https://www.jlab.org/JANA/jana_doc_latest/class_j_streaming_event_source.html>`_ : A starting point for structured, composable streaming
* `JTransport <https://www.jlab.org/JANA/jana_doc_latest/struct_j_transport.html>`_ : An interface for a generic messaging transport
* `JMessage <https://www.jlab.org/JANA/jana_doc_latest/struct_j_message.html>`_: An interface for a stream buffer

User-facing utilities
-----------------------

* `JParameter <https://www.jlab.org/JANA/jana_doc_latest/class_j_parameter.html>`_ : Configure the behavior of components at runtime
* `JLogger <https://www.jlab.org/JANA/jana_doc_latest/struct_j_logger.html>`_ : Publish debugging information to standard out in a structured, convenient, threadsafe way
* `JService <https://www.jlab.org/JANA/jana_doc_latest/struct_j_service.html>`_ : Share external (stateful) services such as calibration constants and magnetic field maps
* `JCsvWriter <https://www.jlab.org/JANA/jana_doc_latest/class_j_csv_writer.html>`_ : Conveniently debug a JFactory by writing its generated JObjects to CSV

Internal services
------------------------

* `JLoggingService <https://www.jlab.org/JANA/jana_doc_latest/class_j_logging_service.html>`_ : Furnish the user with a logger already configured for that particular component
* `JParameterManager <https://www.jlab.org/JANA/jana_doc_latest/class_j_parameter_manager.html>`_ : Furnish the user with parameters extracted from command line flags and configuration files

Parallelism engine
----------------------

* `JProcessingController <https://www.jlab.org/JANA/jana_doc_latest/class_j_processing_controller.html>`_ : The interface which any parallelism engine must adhere to
* `JArrowProcessingController <https://www.jlab.org/JANA/jana_doc_latest/class_j_arrow_processing_controller.html>`_ : The entry point into the “Arrow” engine
* `JWorker <https://www.jlab.org/JANA/jana_doc_latest/class_j_worker.html>`_ : Contains the loop for each worker thread, along with startup/shutdown logic and encapsulated worker state.
* `JScheduler <https://www.jlab.org/JANA/jana_doc_latest/class_j_scheduler.html>`_ : Contains the logic for giving a worker a new assignment
