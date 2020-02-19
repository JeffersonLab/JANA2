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

## Tutorial

This tutorial will walk you through creating a standalone JANA plugin, introducing the key ideas along the way. 
The end result for this example is [available here](https://github.com/nathanwbrei/jana-plugin-example). 

### Introduction

Before we begin, we need to make sure that 
* The JANA library is installed
* The `JANA_HOME` environment variable points to the installation directory
* Your `$PATH` contains `$JANA_HOME/bin`. 

The installation process is described [here](Installation.html). We can quickly test that our install 
was successful by running a builtin benchmarking/scaling test:

```bash
jana -Pplugins=JTest -b
```

We can understand this command as follows:

* `jana` is the default command-line tool for launching JANA. If you would rather create your own executable which 
   uses JANA internally, you are free to do so.
   
* The `-P` flag specifies a configuration parameter, e.g. `-Pnthreads=4` tells JANA to use exactly 4 threads.
   
* `plugins` is the parameter specifying the names of plugins to load, as a comma-separated list (without spaces). 
By default JANA searches for these in `$JANA_HOME/plugins`, although you can also specify full paths.

* `-b` tells JANA to run everything in benchmark mode, i.e. it slowly increases the number of threads while 
measuring the overall throughput. You can cancel processing at any time by pressing Ctrl-C.


### Creating a JANA plugin

With JANA working, we can now create our own plugin. JANA provides a script which generates code skeletons 
to help us get started. If you are working with an existing project such as eJANA or GlueX, you 
should `cd` into `src/plugins` or similar. Otherwise, you can run these commands from your home directory.

We shall name our plugin "QuickTutorial". To generate the skeleton, run
```bash
jana-generate.py plugin QuickTutorial
```

This creates the following directory tree. 

```
QuickTutorial/
├── cmake
│   └── FindJANA.cmake
├── src
│   ├── CMakeLists.txt
│   ├── QuickTutorial.cc
│   ├── QuickTutorialProcessor.cc
│   └── QuickTutorialProcessor.h
└── tests
    ├── catch.hpp
    ├── CMakeLists.txt
    ├── IntegrationTests.cc
    └── TestsMain.cc
```
### Integrating into Existing Build
The skeleton contains a complete stand-alone CMake configuration and can be used as-is to
build the plugin. However, if you want to integrate the plugin into the source of a larger project
such as eJANA then you'll need to make some quick modifications:
* Tell the parent CMakeLists.txt to `add_subdirectory(QuickTutorial)`. 
* Delete `QuickTutorial/cmake` since the project will provide this
* Delete the superfluous project definition inside the root `CMakeLists.txt`

### Building the plugin
We build and run the plugin with the following:

```bash
mkdir build
cd build
cmake ..
make install
jana -Pplugins=QuickTest
```

### Adding an event source

When we run this, we observe that JANA loads the plugin, opens our QuickTutorialProcessor, closes it 
again without processing any events, and exits. This is because there is nothing to do because we haven't
specified any sources. If we are running in the context of an existing project, we can pull in event sources
from other plugins and observe our processor dutifully print out the event number. For now, however, we 
assume that we don't have access to an event source, so we'll create one ourselves. Our first event
source will emit an infinite stream of random data, so we'll name it RandomSource.

```bash
cd src
jana-generate.py JEventSource RandomSource
```

This creates two files, `RandomSource.cc` and `RandomSource.h`, right in the current directory. We'll
need to add them to `CMakeLists.txt` ourselves. Note that we retain complete control over our directory 
structure. In this tutorial, for simplicity, we'll keep all .h and .cc files directly under `src`, except 
for tests, which belong under `tests`. For larger projects, `jana-generate project MyProjectName` creates
a much more complex code skeleton. 

To use our new RandomSource as-is, we need to do three things:
* Add `RandomSource.cc` and `RandomSource.h` to `QuickTutorial_PLUGIN_SOURCES` inside `src/CMakeLists.txt`
* Register our `RandomSource` with JANA inside `QuickTutorial.cc` like this:

```c++
#include <JANA/JApplication.h>

#include "QuickTutorialProcessor.h"
#include "RandomSource.h"                         // <- ADD THIS LINE

extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);

    app->Add(new QuickTutorialProcessor);
    app->Add(new RandomSource("random", app));    // <- ADD THIS LINE
}
}
```
* Rebuild the cmake project, rebuild the plugin target, and install.

When we run the QuickTutorial plugin now, we observe that `QuickTutorialProcessor::Process`
is being called on every event. Because neither the source nor the processor are doing any 
'real work', the events are being processed very quickly. To slow it down, we could add
a delay inside `GetEvent()`:

```std::this_thread::sleep_for(std::chrono::milliseconds(100));```

which would throttle our event rate to around 10 Hz. Also note that `Process` is 'seeing' events slightly 
out-of-order. This is because there are multiple threads running `Process`, which means that we have to be 
careful about how we organize the work we do inside there. This will be discussed in depth later.

<hr>

# Under Development

The rest of this tutorial is still under development ....

### Understanding the code
Let's dive into the skeleton code we've created. 
* `src/QuickTutorialProcessor.*` contains the skeleton of a `JEventProcessor`, which will eventually do
  the bulk of the heavy lifting. For now all it does is print the current event number to a log file.
* `src/QuickTutorial.cc` contains the plugin's entry point, whose only goal right now is to register the
  QuickTutorialProcessor with JANA.
* We have a parallel directory for tests set up so we can start writing tests immediately.










## Problem domain

![Alt JANA Simple system with a single queue](images/queues1.png)
![Alt JANA system with multiple queues](images/queues2.png)

## Parallelism concept

## Factories

![Alt JANA Factory Model](images/factory_model.png)

## Event sources

## Event processors

## More complex topologies

### JEventSource

