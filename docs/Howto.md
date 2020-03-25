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
2.  [Using the JANA command-line program](#using-the-jana-cli)
3.  [Generate code skeletons](#creating-code-skeletons) for projects, plugins, components, etc
4.  Create a service which can be shared between different plugins
5.  Handle both real and simulated data
6.  Handle EPICS data
7.  [Detect when a group of events has finished](howto_group_events.md)
8.  Use JANA with ROOT
9.  Persist the entire DST using ROOT
10. Checkpoint the entire DST using ROOT
11. [Stream data to and from JANA](howto_streaming.html)
12. Build and filter events ("L1 and L2 triggers")
13. Process subevents
14. Migrate from JANA1 to JANA2


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



The most commonly used configuration parameters are below. Your plugins can define their own parameters
and automatically get them from the command line or config file as well.


| Name | Description |
|:-----|:------------|
plugins                   | Comma-separated list of plugin filenames. JANA will look for these on the `$JANA_PLUGIN_PATH`
nthreads                  | Size of thread team (Defaults to the number of cores on your machine)
jana:debug_plugin_loading | Log additional information in case JANA isn't finding your plugins
jtest:nsamples            | Number of randomly-generated events to process when running JTEST



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





