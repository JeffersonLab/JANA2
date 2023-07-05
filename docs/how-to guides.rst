How-to...
=============
This section walks the user through specific steps for solving a real-world problem.

Table of contents
-----------------
1. Download and install JANA
2. Use the JANA command-line program
3. Configure JANA
4. Benchmark a JANA program
5. Generate code skeletons for projects, plugins, components, etc
6. Run the JANA Status/Control/Debugger GUI
7. Work with factory metadata for collecting statistics, etc
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
  :widths: 25 10 20 50
  :header-rows: 1

   * - Name
     - Type
     - Default
     - Description
   * - benchmark:nsamples
     - int
     - 15
     - Number of measurements made for each thread count
   * - 
     - 
     - 
     - 
