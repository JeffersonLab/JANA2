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

How To...
=========
This section walks the user through specific steps for solving a real-world problem. 

Table of contents
-----------------

1.  [Download](Download.html) and [install](Installation.html) JANA
2.  [Use the JANA command-line program](#using-the-jana-cli)
3.  [Configure JANA](#configuring-jana)
6.  [Benchmark a JANA program](benchmarking.html)
4.  [Generate code skeletons](#creating-code-skeletons) for projects, plugins, components, etc
5.  [Run the JANA Status/Control/Debugger GUI](#run-the-status-control-debugger-gui)
6.  [Work with factory metadata](#using-factory-metadata) for collecting statistics, etc
7.  Create a service which can be shared between different plugins
8.  Handle both real and simulated data
9.  Handle EPICS data
10.  [Detect when a group of events has finished](howto_group_events.md)
11.  Use JANA with ROOT
12.  Persist the entire DST using ROOT
13. Checkpoint the entire DST using ROOT
14. [Stream data to and from JANA](howto_streaming.html)
15. Build and filter events ("L1 and L2 triggers")
16. Process subevents
17. Migrate from JANA1 to JANA2


Using the JANA CLI
------------------

JANA is typically run like this:

~~~ bash
$JANA_HOME/bin/jana -Pplugins=JTest -Pnthreads=8 ~/data/inputfile.txt
~~~

Note that the JANA executable won't do anything until you provide plugins.
A simple plugin is provided called JTest, which verifies that everything is working and optionally does a quick
performance benchmark. Additional simple plugins are provided in `src/examples`. Instructions on how to write your
own are given in the Tutorial section.

Along with specifying plugins, you need to specify the input files containing the events you wish to process.
Note that JTest ignores these and crunches randomly generated data instead.


The command-line flags are:

| Short | Long | Meaning  |
|:------|:-----|:---------|
| -h    | --help               | Display help message |
| -v    | --version            | Display version information |
| -c    | --configs            | Display configuration parameters |
| -l    | --loadconfigs <file> | Load configuration parameters from file |
| -d    | --dumpconfigs <file> | Dump configuration parameters to file |
| -b    | --benchmark          | Run JANA in benchmark mode |
| -P    |                      | Specify a configuration parameter (see below) |



Configuring JANA
----------------

JANA provides a parameter manager so that configuration options may be controlled via code, command-line args, and 
config files in a consistent and self-documenting way. Plugins are free to request any existing parameters or register
their own. 

The following configuration options are used most commonly:

| Name | Type | Description |
|:-----|:-----|:------------|
nthreads                  | int     | Size of thread team (Defaults to the number of cores on your machine)
plugins                   | string  | Comma-separated list of plugin filenames. JANA will look for these on the `$JANA_PLUGIN_PATH`
plugins_to_ignore         | string  | This removes plugins which had been specified in `plugins`. 
event_source_type         | string  | Manually override JANA's decision about which JEventSource to use
jana:nevents              | int     | Limit the number of events each source may emit
jana:nskip                | int     | Skip processing the first n events from each event source
jana:extended_report      | bool    | The amount of status information to show while running
jana:status_fname         | string  | Named pipe for retrieving status information remotely


JANA has its own logger. You can control the verbosity of different components using 
the parameters `log:off`, `log:fatal`, `log:error`, `log:warn`, `log:info`, `log:debug`, and `log:trace`.
The following example shows how you would increase the verbosity of JPluginLoader and JComponentManager:
```
jana -Pplugins=JTest -Plog:debug=JPluginLoader,JComponentManager
```

The `JTest` plugin lets you test JANA's performance for different workloads. It simulates a typical reconstruction pipeline with four stages: parsing, disentangling, tracking, and plotting. Parsing and plotting are sequential, whereas disentangling and tracking are parallel. Each stage reads all of the data written during the previous stage. The time spent and bytes written (and random variation thereof) are set using the following parameters:
 
| Name | Type | Default | Description |
|:-----|:-----|:------------|:--------|
jtest:parser_ms | int | 0 | Time spent during parsing
jtest:parser_spread | int | 0.25 | Spread of time spent during parsing
jtest:parser_bytes | int | 2000000 | Bytes written during parsing
jtest:parser_bytes_spread | double | 0.25 | Spread of bytes written during parsing
jtest:disentangler_ms | int | 20 | Time spent during disentangling
jtest:disentangler_spread | double | 0.25 | Spread of time spent during disentangling
jtest:disentangler_bytes | int | 500000 | Bytes written during disentangling
jtest:disentangler_bytes_spread | double | 0.25 | Spread of bytes written during disentangling
jtest:tracker_ms | int | 200 | Time spent during tracking
jtest:tracker_spread | double | 0.25 | Spread of time spent during tracking
jtest:tracker_bytes | int | 1000 | Bytes written during tracking
jtest:tracker_bytes_spread | double | 0.25 | Spread of bytes written during tracking
jtest:plotter_ms | int | 20 | Time spent during plotting
jtest:plotter_spread | double | 0.25 | Spread of time spent during plotting
jtest:plotter_bytes | int | 1000 | Bytes written during plotting
jtest:plotter_bytes_spread | int | 0.25 | Spread of bytes written during plotting



The following parameters are used for benchmarking:

| Name | Type | Default | Description |
|:-----|:-----|:------------|:--------|
benchmark:nsamples    | int    | 15 | Number of measurements made for each thread count
benchmark:minthreads  | int    | 1  | Minimum thread count
benchmark:maxthreads  | int    | ncores | Maximum thread count
benchmark:threadstep  | int    | 1  | Thread count increment
benchmark:resultsdir  | string | JANA_Test_Results | Directory name for benchmark test results


The following parameters are more advanced, but may come in handy when doing performance tuning:

| Name | Type | Default | Description |
|:-----|:-----|:------------|:--------|
jana:engine                       | int  | 0        | Which parallelism engine to use. 0: JArrowProcessingController. 1: JDebugProcessingController.
jana:event_pool_size              | int  | nthreads | The number of events which may be in-flight at once
jana:limit_total_events_in_flight | bool | 1        | Whether the number of in-flight events should be limited
jana:affinity                     | int  | 0        | Thread pinning strategy. 0: None. 1: Minimize number of memory localities. 2: Minimize number of hyperthreads.
jana:locality                     | int  | 0        | Memory locality strategy. 0: Global. 1: Socket-local. 2: Numa-domain-local. 3. Core-local. 4. Cpu-local
jana:enable_stealing              | bool | 0        | Allow threads to pick up work from a different memory location if their local mailbox is empty.
jana:event_queue_threshold        | int  | 80       | Mailbox buffer size
jana:event_source_chunksize       | int  | 40       | Reduce mailbox contention by chunking work assignments
jana:event_processor_chunksize    | int  | 1        | Reduce mailbox contention by chunking work assignments


Creating code skeletons
-----------------------

JANA provides a script, `$JANA_HOME/bin/jana-generate.py`, which generates code skeletons for 
different kinds of JANA components, but also entire project structures. These are intended to 
compile and run with zero or minimal modification, to provide all of the boilerplate needed, and
to include comments explaining what each piece of boilerplate does and what the user is expected
to add. The aim is to demonstrate idiomatic usage of the JANA framework and reduce the learning
curve as much as possible.

#### Complete projects
The 'project' skeleton lays out the recommended structure for a complex experiment with multiple 
plugins, a domain model which is shared between plugins, and a custom executable. In general, 
each experiment is expected to have one project.

```jana-generate.py project ProjectName```

#### Project plugins
Project plugins are used to modularize some functionality within the context of an existing project.
Not only does this help separate concerns, so that many members of a collaboration can work together
without interfering with another, but it also helps manage the complexity arising from build dependencies.
Some scientific software stubbornly refuses to build on certain platforms, and plugins are a much cleaner
solution than the traditional mix of environment variables, build system variables, and preprocessor macros.
Project plugins include one JEventProcessor by default.

```jana-generate.py ProjectPlugin PluginNameInCamelCase```


#### Mini plugins
Mini plugins are project plugins which have been stripped down to a single `cc` file. They are useful
when someone wants to do a quick analysis and doesn't need or want the additional boilerplate. They
include one JEventProcessor with support for ROOT histograms. There are two options: 

```
jana-generate.py MiniStandalonePlugin PluginNameInCamelCase
jana-generate.py MiniProjectPlugin PluginNameInCamelCase
```

#### Standalone plugins 
Standalone plugins are useful for getting started quickly. They are also effective when someone wishes to 
integrate with an existing project, but want their analyses to live in a separate repository.

```jana-generate.py StandalonePlugin PluginNameInCamelCase```
  
#### Executables
Executables are useful when using the provided `$JANA_HOME/bin/jana` is inconvenient. This may be
because the project is sufficiently simple that multiple plugins aren't even needed, or because the project is 
sufficiently complex that specialized configuration is needed before loading any other plugins.

```jana-generate.py Executable ExecutableNameInCamelCase```

#### JEventSources

```jana-generate.py JEventSource NameInCamelCase```

#### JEventProcessors

```jana-generate.py JEventProcessor NameInCamelCase```

#### JEventProcessors which output to ROOT

This JEventProcessor includes the boilerplate for creating a ROOT histogram in a specific
virtual subdirectory of a TFile. If this TFile is shared among different `JEventProcessors`,
it should be encapsulated in a JService. Otherwise, it can be specified as a simple parameter.
We recommend naming the subdirectory after the plugin name. E.g. a `trk_eff` plugin contains 
a `TrackingEfficiencyProcessor` which writes all of its results to the `trk_eff` subdirectory 
of the TFile. 
 
```jana-generate.py RootEventProcessor ProcessorNameInCamelCase directory_name_in_snake_case```

Note that this script, like the others, does not update your `CMakeLists.txt`. Not only will you 
need to add the file to `PluginName_PLUGIN_SOURCES`, but you may need to add ROOT as a 
dependency if your project hasn't yet:

```
find_package(ROOT)
include_directories(${ROOT_INCLUDE_DIRS})
link_directories(${ROOT_LIBRARY_DIR})
target_link_libraries(${PLUGIN_NAME} ${ROOT_LIBRARIES})
``` 

#### JFactories

Because JFactories are templates parameterized by the type of JObjects they produce, we need two arguments
to generate them. The naming convention is left up to the user, but the following is recommended. If the 
JObject name is 'RecoTrack', and the factory uses Genfit under the hood, the factory name should be
'RecoTrackFactory_Genfit'. 

```jana-generate.py JFactory JFactoryNameInCamelCase JObjectNameInCamelCase```

Run the Status Control Debugger GUI
-----------------------------------

The JANA Status/Control/Debugger GUI can be a useful tool for probing a running process. Details can
be found on the [dedicated page for the GUI](GUI_Monitor_Debug.md)


Using factory metadata
----------------------

The `JFactoryT<T>` interface abstracts the creation logic for a vector of n objects of type `T`. However, often
we also care about single pieces of data associated with the same computation. For instance, a track fitting factory
might want to return statistics about how many fits succeeded and failed. 

A naive solution is to put member variables on the factory and then access them from a `JEventProcessor` 
by obtaining the `JFactoryT<T>` via `GetFactory<>` and performing a dynamic cast to the underlying factory type. 
Although this works, it means that that factory can no longer be swapped with an alternate version without modifying
the calling code. This degrades the whole project's ability to take advantage of the plugin architecture and hurts 
its overall code quality.

Instead, we recommend using the `JMetadata` template trait. Each `JFactoryT<T>` not only produces a vector of `T`, 
but also a singular `JMetadata<T>` struct whose contents can be completely arbitrary, but cannot be redefined for a
particular T. All `JFactoryT<T>` for some `T` will use it. 

An example project demonstrating usage of JMetadata can be found under `examples/MetadataExample`. 






