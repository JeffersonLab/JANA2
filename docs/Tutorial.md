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

This tutorial will walk you through creating a standalone JANA plugin, introducing the key ideas along the way. 
All of this code for this example is [available here](https://github.com/nathanwbrei/jana-plugin-example).

The first thing we need to do is install the JANA library. This is done just like any other CMake project. 
We choose an installation directory by setting the environment variable `$JANA_HOME`. For now, this will be 
under `JANA2/install`, although if we want a system-wide installation, it should be `/usr/local`.

```
mkdir ~/tutorial
cd ~/tutorial
git clone https://github.com/JeffersonLab/JANA2
cd JANA2
mkdir install
export JANA_HOME=`pwd`/install
mkdir build
cd build
cmake ..
make install
```

We can quickly test that our install works by running
```
$JANA_HOME/bin/jana -Pplugins=JTest -b
```
This will launch a plugin that will benchmark the performance of your system and measure how
JANA's throughput scales with the number of threads being used. You can cancel processing any time 
by pressing Ctrl-C.

With JANA working, we can now create our own standalone plugin. 
```
cd ~/tutorial
mkdir sample_plugin
git init
```

You can use JANA with any build system. For this tutorial, we shall use CMake. JANA provides a FindJANA.cmake script
which tells CMake where to find everything relative to `$JANA_HOME`.
```
cd ~/tutorial/sample_plugin
mkdir cmake
cp JANA2/cmake/FindJANA.cmake sample_plugin/cmake
touch CMakeLists.txt
```

We are going to use a very simple directory structure for now. When it comes time to put together a larger project,
use [this example](https://github.com/nathanwbrei/jana-project-example) as a guide.
```
CMakeLists.txt
cmake
    FindJANA.cmake
src
    CMakeLists.txt
    sample_plugin.cc
```

The root CMakeLists.txt file tells CMake that we require C++14 and PIC, where to find . The code which goes into
the 

```
cmake_minimum_required(VERSION 3.13)
project(sample_jana_project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

add_subdirectory(src)
```








## Problem domain

![Alt JANA Simple system with a single queue](images/queues1.png)
![Alt JANA system with multiple queues](images/queues2.png)

## Parallelism concept

## Factories

![Alt JANA Factory Model](images/factory_model.png)

## Event sources

## Event processors

## More complex topologies

### JObjects

JObjects are data containers for specific results. JObjects are close to being plain-old structs, except they include
some extra functionality for creating and traversing associations with other JObjects. They are immutable once 
outside of the JFactory created them. It may be beneficial to use multiple inheritance in order to take gain additional
 functionality, e.g. to delegate persistence to ROOT.

### JEventSource

