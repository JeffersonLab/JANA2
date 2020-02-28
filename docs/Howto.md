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
2.  [Generate code skeletons](#Creating-code-skeletons) for projects, plugins, components, etc]
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
Mini plugins are project plugins which have been stripped down to a single `cpp` file. They are useful
when someone wants to do a quick analysis and doesn't need or want the additional boilerplate. They
include one JEventProcessor. 

```jana-generate.py MiniPlugin PluginNameInCamelCase```

#### Standalone plugins 
Standalone plugins are useful for getting started quickly. They are also effective when someone wishes to 
integrate with an existing project, but want their analyses to live in a separate repository.

```jana-generate.py plugin PluginNameInCamelCase```
  
#### Executables
Executables are useful when using the provided `$JANA_HOME/bin/jana` is inconvenient. This may be
because the project is sufficiently simple that multiple plugins aren't even needed, or because the project is 
sufficiently complex that specialized configuration is needed before loading any other plugins.

```jana-generate.py executable ExecutableNameInCamelCase```

#### JEventSources

```jana-generate.py JEventSource NameInCamelCase```

#### JEventProcessors

```jana-generate.py JEventProcessor NameInCamelCase```

#### JEventProcessors which output to ROOT

```jana-generate.py RootEventProcessor NameInCamelCase```

#### JFactories

Because JFactories are templates parameterized by the type of JObjects they produce, we need two arguments
to generate them. The naming convention is left up to the user, but the following is recommended. If the 
JObject name is 'RecoTrack', and the factory uses Genfit under the hood, the factory name should be
'RecoTrackFactory_Genfit'. 

```jana-generate.py JFactory JFactoryNameInCamelCase JObjectNameInCamelCase```





