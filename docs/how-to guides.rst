How-to guides
=============
This section walks the user through specific steps for solving a real-world problem.

Table of contents
-----------------
1. Download and `install <https://jana.readthedocs.io/en/latest/how-to%20guides.html#building-jana>`_ JANA
2. `Use the JANA command-line program <https://jana.readthedocs.io/en/latest/how-to%20guides.html#using-the-jana-cli>`_
3. `Configure JANA <https://jana.readthedocs.io/en/latest/how-to%20guides.html#configuring-jana>`_
4. Benchmark a JANA program
5. `Generate code skeletons <https://jana.readthedocs.io/en/latest/how-to%20guides.html#creating-code-skeletons>`_ for projects, plugins, components, etc
6. `Run the JANA Status/Control/Debugger GUI <https://jana.readthedocs.io/en/latest/how-to%20guides.html#run-the-status-control-debugger-gui>`_
7. `Work with factory metadata <https://jana.readthedocs.io/en/latest/how-to%20guides.html#using-factory-metadata>`_ for collecting statistics, etc
8. Create a service which can be shared between different plugins
9. Handle both real and simulated data
10. Handle EPICS data
11. Detect when a group of events has finished
12. Use JANA with ROOT
13. Persist the entire DST using ROOT
14. Checkpoint the entire DST using ROOT
15. Stream data to and from JANA
16. Build and filter events (“L1 and L2 triggers”)
17. Process subevents
18. Migrate from JANA1 to JANA2

Building JANA
~~~~~~~~~~~~~~
First, set your :py:func:`$JANA_HOME` environment variable. This is where the executables, libraries, headers, and plugins get installed. (It is also where we will clone the source). CMake will install to :py:func:`$JANA_HOME` if it is set (it will install to :py:func:`${CMAKE_BINARY_DIR}/install` if not). Be aware that although CMake usually defaults :py:func:`CMAKE_INSTALL_PREFIX` to :py:func:`/usr/local`, we have disabled this because we rarely want this in practice, and we don’t want the build system picking up outdated headers and libraries we installed to :py:func:`/usr/local` by accident. If you want to set :py:func:`JANA_HOME=/usr/local`, you are free to do so, but you must do so deliberately.

Next, set your build directory. This is where CMake’s caches, logs, intermediate build artifacts, etc go. The convention is to name it :py:func:`build` and put it in the project’s root directory. If you are using CLion, it will automatically create a :py:func:`cmake-build-debug` directory which works just fine.

Finally, you can cd into your build directory and build and install everything the usual CMake way.

.. code-block:: console 

  export JANA_VERSION=v2.0.5                    # Convenient to set this once for specific release
  export JANA_HOME=${PWD}/JANA${JANA_VERSION}   # Set full path to install dir
  
  git clone https://github.com/JeffersonLab/JANA2 --branch ${JANA_VERSION} ${JANA_HOME}  # Get JANA2
  
  mkdir build                                   # Set build dir
  cd build
  cmake3 ${JANA_HOME}  # Generate makefiles     # Generate makefiles
  make -j8 install                              # Build (using 8 threads) and install
  
  source ${JANA_HOME}/bin/jana-this.sh          # Set PATH (and other envars)
  jana -Pplugins=JTest                          # Run JTest plugin to verify successful install

Note: If you want to use a compiler other than the default one on your system, it is not enough to modify your $PATH, as CMake ignores this by design. You either need to set the :py:func:`CXX` environment variable or the :py:func:`CMAKE_CXX_COMPILER` CMake variable.

By default, JANA will look for plugins under :py:func:`$JANA_HOME/plugins`. For your plugins to propagate here, you have to :py:func:`install` them. If you don’t want to do that, you can also set the environment variable :py:func:`$JANA_PLUGIN_PATH` to point to the build directory of your project. JANA will report where exactly it went looking for your plugins and what happened when it tried to load them if you set the JANA config :py:func:`jana:debug_plugin_loading=1`.

.. code-block:: console 

  jana -Pplugins=JTest -Pjana:debug_plugin_loading=1

Using JANA in a CMake project
______________________________
To use JANA in a CMake project, simply add :py:func:`$JANA_HOME/lib/cmake/JANA` to your :py:func:`CMAKE_PREFIX_PATH`, or alternatively, set the CMake variable :py:func:`JANA_DIR=$JANA_HOME/lib/cmake/JANA`.

Using JANA in a non-CMake project
__________________________________

To use JANA in a non-CMake project:

Source :py:func:`$JANA_HOME/bin/jana-this.sh` to set the environment variables needed for JANA’s dependencies
Use :py:func:`$JANA_HOME/bin/jana-config --cflags` to obtain JANA’s compiler flags
Use :py:func:`$JANA_HOME/bin/jana_config --libs` to obtain JANA’s linker flags

Using the JANA CLI
-------------------

JANA is typically run like this:

.. code-block:: console 

  $JANA_HOME/bin/jana -Pplugins=JTest -Pnthreads=8 ~/data/inputfile.txt

Note that the JANA executable won’t do anything until you provide plugins. A simple plugin is provided called JTest, which verifies that everything is working and optionally does a quick performance benchmark. Additional simple plugins are provided in py:func:`src/examples`. Instructions on how to write your own are given in the Tutorial section.

Along with specifying plugins, you need to specify the input files containing the events you wish to process. Note that JTest ignores these and crunches randomly generated data instead.

The command-line flags are:

.. list-table:: 
   :widths: 10 25 50
   :header-rows: 1

   * - Short
     - Long
     - Meaning
   * - -h
     - –help
     - 	Display help message
   * - -v
     - 	–version
     - 	Display version information
   * - -c
     - 	–configs
     - 	Display configuration parameters
   * - -l
     - 	–loadconfigs
     - 	Load configuration parameters from file
   * - -d
     - –dumpconfigs
     - Dump configuration parameters to file
   * - -b
     - 	–benchmark
     - 	Run JANA in benchmark mode
   * - -P
     - 
     - Specify a configuration parameter (see below)

Configuring JANA
-----------------
JANA provides a parameter manager so that configuration options may be controlled via code, command-line args, and config files in a consistent and self-documenting way. Plugins are free to request any existing parameters or register their own.

The following configuration options are used most commonly:

.. list-table:: 
   :widths: 25 25 50
   :header-rows: 1

   * - Name
     - Type
     - Descriptioin
   * - nthreads
     - int
     - Size of thread team (Defaults to the number of cores on your machine)
   * - plugins
     - string
     - Comma-separated list of plugin filenames. JANA will look for these on the :py:func:`$JANA_PLUGIN_PATH`
   * - plugins_to_ignore
     - string
     - This removes plugins which had been specified in :py:func:`plugins`.
   * - event_source_type
     - string
     - Manually override JANA’s decision about which JEventSource to use
   * - jana:nevents
     - int	
     - 	Limit the number of events each source may emit
   * - jana:nskip
     - int	
     - 	Skip processing the first n events from each event source
   * - jana:extended_report
     - bool
     - 	The amount of status information to show while running
   * - jana:status_fname
     - string
     - 	Named pipe for retrieving status information remotely

JANA has its own logger. You can control the verbosity of different components using the parameters :py:func:`log:off`, :py:func:`log:fatal`, :py:func:`log:error`, :py:func:`log:warn`, :py:func:`log:info`, :py:func:`log:debug`, and :py:func:`log:trace`. The following example shows how you would increase the verbosity of JPluginLoader and JComponentManager:

.. code-block:: console 

  jana -Pplugins=JTest -Plog:debug=JPluginLoader,JComponentManager

The following parameters are used for benchmarking:

.. list-table:: 
   :widths: 25 10 25 50
   :header-rows: 1

   * - Name
     - Type
     - Default
     - Description
   * - benchmark:nsamples
     - int
     - 15
     - Number of measurements made for each thread count
   * - benchmark:minthreads
     - int
     - 1
     - Minimum thread count
   * - benchmark:maxthread
     - int
     - ncores
     - Maximum thread count
   * - benchmark:threadstep
     - int
     - 1
     - Thread count increment
   * - benchmark:resultsdir
     - string
     - JANA_Test_Results
     - Directory name for benchmark test results

The following parameters may come in handy when doing performance tuning:

.. list-table:: 
   :widths: 25 10 25 50
   :header-rows: 1

   * - Name
     - Type
     - Default
     - Description
   * - jana:engine
     - int
     - 0
     - Which parallelism engine to use. 0: JArrowProcessingController. 1: JDebugProcessingController.
   * - jana:event_pool_size
     - int
     - nthreads
     - The number of events which may be in-flight at once
   * - jana:limit_total_events_in_flight
     - bool
     - 1
     - Whether the number of in-flight events should be limited
   * - jana:affinity
     - int
     - 0
     - Thread pinning strategy. 0: None. 1: Minimize number of memory localities. 2: Minimize number of hyperthreads.
   * - jana:locality
     - int
     - 0
     - Memory locality strategy. 0: Global. 1: Socket-local. 2: Numa-domain-local. 3. Core-local. 4. Cpu-local
   * - jana:enable_stealing
     - bool
     - 0
     - Allow threads to pick up work from a different memory location if their local mailbox is empty.
   * - jana:event_queue_threshold
     - int
     - 80
     - Mailbox buffer size
   * - jana:event_source_chunksize
     - int
     - 40
     - 	Reduce mailbox contention by chunking work assignments
   * - jana:event_processor_chunksize
     - int
     - 1
     - Reduce mailbox contention by chunking work assignments

Creating code skeletons
------------------------
JANA provides a script, :py:func:`$JANA_HOME/bin/jana-generate.py`, which generates code skeletons for different kinds of JANA components, but also entire project structures. These are intended to compile and run with zero or minimal modification, to provide all of the boilerplate needed, and to include comments explaining what each piece of boilerplate does and what the user is expected to add. The aim is to demonstrate idiomatic usage of the JANA framework and reduce the learning curve as much as possible.

Complete projects
_________________
The ‘project’ skeleton lays out the recommended structure for a complex experiment with multiple plugins, a domain model which is shared between plugins, and a custom executable. In general, each experiment is expected to have one project.

:py:func:`jana-generate.py project ProjectName`

Project plugins
_________________
Project plugins are used to modularize some functionality within the context of an existing project. Not only does this help separate concerns, so that many members of a collaboration can work together without interfering with another, but it also helps manage the complexity arising from build dependencies. Some scientific software stubbornly refuses to build on certain platforms, and plugins are a much cleaner solution than the traditional mix of environment variables, build system variables, and preprocessor macros. Project plugins include one JEventProcessor by default.

:py:func:`jana-generate.py ProjectPlugin PluginNameInCamelCase`

Mini plugins
______________
Mini plugins are project plugins which have been stripped down to a single cc file. They are useful when someone wants to do a quick analysis and doesn’t need or want the additional boilerplate. They include one JEventProcessor with support for ROOT histograms. There are two options:

.. code-block:: console 

  jana-generate.py MiniStandalonePlugin PluginNameInCamelCase
  jana-generate.py MiniProjectPlugin PluginNameInCamelCase

Standalone plugins
___________________
Standalone plugins are useful for getting started quickly. They are also effective when someone wishes to integrate with an existing project, but want their analyses to live in a separate repository.

:py:func:`jana-generate.py StandalonePlugin PluginNameInCamelCase`

Executables
_____________
Executables are useful when using the provided :py:func:`$JANA_HOME/bin/jana` is inconvenient. This may be because the project is sufficiently simple that multiple plugins aren’t even needed, or because the project is sufficiently complex that specialized configuration is needed before loading any other plugins.

:py:func:`jana-generate.py Executable ExecutableNameInCamelCase`

JEventSources
_____________
:py:func:`jana-generate.py JEventSource NameInCamelCase`

JEventProcessors
________________
:py:func:`jana-generate.py JEventProcessor NameInCamelCase`

JEventProcessors which output to ROOT
_____________________________________
This JEventProcessor includes the boilerplate for creating a ROOT histogram in a specific virtual subdirectory of a TFile. If this TFile is shared among different :py:func:`JEventProcessors`, it should be encapsulated in a JService. Otherwise, it can be specified as a simple parameter. We recommend naming the subdirectory after the plugin name. E.g. a :py:func:`trk_eff` plugin contains a :py:func:`TrackingEfficiencyProcessor` which writes all of its results to the :py:func:`trk_eff` subdirectory of the TFile.

:py:func:`jana-generate.py RootEventProcessor ProcessorNameInCamelCase`
:py:func:`directory_name_in_snake_case`

Note that this script, like the others, does not update your :py:func:`CMakeLists.txt`. Not only will you need to add the file to :py:func:`PluginName_PLUGIN_SOURCES`, but you may need to add ROOT as a dependency if your project hasn’t yet:

.. code-block:: console

  find_package(ROOT)
  include_directories(${ROOT_INCLUDE_DIRS})
  link_directories(${ROOT_LIBRARY_DIR})
  target_link_libraries(${PLUGIN_NAME} ${ROOT_LIBRARIES})

JFactories
___________
Because JFactories are templates parameterized by the type of JObjects they produce, we need two arguments to generate them. The naming convention is left up to the user, but the following is recommended. If the JObject name is ‘RecoTrack’, and the factory uses Genfit under the hood, the factory name should be ‘RecoTrackFactory_Genfit’.

:py:func:`jana-generate.py JFactory JFactoryNameInCamelCase JObjectNameInCamelCase`

Run the Status Control Debugger GUI
-------------------------------------
The JANA Status/Control/Debugger GUI can be a useful tool for probing a running process. Details can be found on the dedicated page for the GUI

Using factory metadata
----------------------
The :py:func:`JFactoryT<T>` interface abstracts the creation logic for a vector of n objects of type :py:func:`T`. However, often we also care about single pieces of data associated with the same computation. For instance, a track fitting factory might want to return statistics about how many fits succeeded and failed.

A naive solution is to put member variables on the factory and then access them from a :py:func:`JEventProcessor` by obtaining the :py:func:`JFactoryT<T>` via :py:func:`GetFactory<>` and performing a dynamic cast to the underlying factory type. Although this works, it means that that factory can no longer be swapped with an alternate version without modifying the calling code. This degrades the whole project’s ability to take advantage of the plugin architecture and hurts its overall code quality.

Instead, we recommend using the :py:func:`JMetadata` template trait. Each :py:func:`JFactoryT<T>` not only produces a vector of :py:func:`T`, but also a singular :py:func:`JMetadata<T>` struct whose contents can be completely arbitrary, but cannot be redefined for a particular T. All :py:func:`JFactoryT<T>` for some :py:func:`T` will use it.

An example project demonstrating usage of JMetadata can be found under :py:func:`examples/MetadataExample`.
