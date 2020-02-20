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

# Tutorial

This tutorial will walk you through creating a standalone JANA plugin, introducing the key ideas along the way. 
The end result for this example is [available here](https://github.com/nathanwbrei/jana-plugin-example). 

## Introduction

Before we begin, we need to make sure that 
* The JANA library is installed
* The `JANA_HOME` environment variable points to the installation directory
* Your `$PATH` contains `$JANA_HOME/bin`. 

The installation process is described [here](Installation.html). We can quickly test that our install 
was successful by running a builtin benchmarking/scaling test:

```
jana -Pplugins=JTest -b
```

We can understand this command as follows:

* `jana` is the default command-line tool for launching JANA. If you would rather create your own executable which 
   uses JANA internally, you are free to do so.
   
* The `-P` flag specifies a configuration parameter, e.g. `-Pjana:debug_plugin_loading=1` tells JANA to log
   detailed information about where the plugin loader went looking and what it found.
   
* `plugins` is the parameter specifying the names of plugins to load, as a comma-separated list (without spaces). 
By default JANA searches for these in `$JANA_HOME/plugins`, although you can also specify full paths.

* `-b` tells JANA to run everything in benchmark mode, i.e. it slowly increases the number of threads while 
measuring the overall throughput. You can cancel processing at any time by pressing Ctrl-C.


## Creating a JANA plugin

With JANA working, we can now create our own plugin. JANA provides a script which generates code skeletons 
to help us get started. If you are working with an existing project such as eJANA or GlueX, you 
should `cd` into `src/plugins` or similar. Otherwise, you can run these commands from your home directory.

We shall name our plugin "QuickTutorial". To generate the skeleton, run
```
jana-generate.py plugin QuickTutorial
```

This creates the following directory tree. Examine the files `QuickTutorial.cc`, which provides the plugin
 entry point, and `QuickTutorialProcessor.cc`, which is where the majority of the work happens. The generated
 files include lots of comments providing helpful hints on their use. 

```
QuickTutorial/
├── CMakeLists.txt
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

## Integrating into an existing project
The skeleton contains a complete stand-alone CMake configuration and can be used as-is to
build the plugin. However, if you want to integrate the plugin into the source of a larger project
such as eJANA then you'll need to make some quick modifications:
* Tell the parent CMakeLists.txt to `add_subdirectory(QuickTutorial)`
* Delete `QuickTutorial/cmake` since the project will provide this
* Delete the superfluous project definition inside the root `CMakeLists.txt`

## Building the plugin
We build and run the plugin with the following:

```
mkdir build
cd build
cmake ..
make install
jana -Pplugins=QuickTest
```

## Adding an event source

When we run this, we observe that JANA loads the plugin, opens our QuickTutorialProcessor, closes it 
again without processing any events, and exits. This is because there is nothing to do because we haven't
specified any sources. If we are running in the context of an existing project, we can pull in event sources
from other plugins and observe our processor dutifully print out the event number. For now, however, we 
assume that we don't have access to an event source, so we'll create one ourselves. Our first event
source will emit an infinite stream of random data, so we'll name it RandomSource.

```
cd src
jana-generate.py JEventSource RandomSource
```

This creates two files, `RandomSource.cc` and `RandomSource.h`, in the current directory. We'll
need to add them to `CMakeLists.txt` ourselves. Note that we retain complete control over our directory 
structure. In this tutorial, for simplicity, we'll keep all .h and .cc files directly under `src`, except 
for tests, which belong under `tests`. For larger projects, `jana-generate project MyProjectName` creates
a much more complex code skeleton. 

To use our new RandomSource as-is, we need to do three things:
* Add `RandomSource.cc` and `RandomSource.h` to `QuickTutorial_PLUGIN_SOURCES` inside `src/CMakeLists.txt`
* Register our `RandomSource` with JANA inside `QuickTutorial.cc` like this:

```
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
is being called on every event. Note that `Process` is 'seeing' events slightly out-of-order. This is 
because there are multiple threads running `Process`, which means that we have to be careful about how 
we organize the work we do inside there. This will be discussed in depth later.

## Configuring an event source

Because neither the source nor the processor are doing any 'real work', the events are being processed 
very quickly. To throttle the rate events get emitted, to whatever frequency we like, we can add a delay 
inside `GetEvent`. Perhaps we'd like to set the emit frequency at runtime. First, we declare a member variable on 
`RandomSource`, initializing it to our preferred default value:

```
class RandomSource : public JEventSource {
    int m_max_emit_freq_hz = 100;             // <- ADD THIS LINE

public:
    RandomSource(std::string resource_name, JApplication* app);
    virtual ~RandomSource() = default;
    void Open() override;
    void GetEvent(std::shared_ptr<JEvent>) override;
};
```

Next we sync the variable with the parameter manager inside `Open`. We do this by calling 
`JApplication::SetDefaultParameter`, which tells JANA to look among its configuration parameters for one 
called "random_source:max_emit_freq_hz". If it finds one, it 
sets `m_max_emit_freq_hz` to the value it found. Otherwise, it leaves the variable alone. JANA remembers 
all such 'default parameters' along with their default values so that it can report them and generate config
files. Note that we conventionally prefix our parameter names with the name of the requesting component or 
plugin. This helps prevent namespace collisions. 

```
void RandomSource::Open() {
    JApplication* app = GetApplication();
    app->SetDefaultParameter("random_source:max_emit_freq_hz",            // ADD ME
                             m_max_emit_freq_hz,                          // ADD ME
                             "Maximum event rate [Hz] for RandomSource"); // ADD ME
}
```

We can now use the value of `m_max_emit_freq_hz`, confident that it is consistent with the current 
runtime configuration:

```
void RandomSource::GetEvent(std::shared_ptr <JEvent> event) {

    /// Configure event and run numbers
    static size_t current_event_number = 1;
    event->SetEventNumber(current_event_number++);
    event->SetRunNumber(22);

    /// Slow down event source                                           // ADD ME
    auto delay_ms = std::chrono::milliseconds(1000/m_max_emit_freq_hz);  // ADD ME
    std::this_thread::sleep_for(delay_ms);                               // ADD ME
}
```

Finally, we can set this parameter on the command line and observe the throughput change accordingly:

```
jana -Pplugins=QuickTutorial -Prandom_source:max_emit_freq_hz=10
```


## Creating JObjects

So far `RandomSource` has been emitting events with no data attached. Now we'd like to have them 
emit randomly generated 'Hit' objects which simulate the readout from a detector. First, we need 
to set up our data model. Although we can insert pointers of any kind into our `JEvent`, we strongly
recommend using `JObjects` for reasons we will discuss later. 

```
cd src
jana-generate.py JObject Hit
```

JObjects are meant to be plain-old data. For this tutorial we pretend that our detector consists of a 3D grid
of sensors, each of which measures some energy at some time. Note that we are declaring `Hit` to be a `struct`
instead of a `class`. This is because `JObjects` should be lightweight containers with no creation logic and 
no invariants which need to be encapsulated. JObjects are free to contain pointers to arbitrary data types and
nested STL containers, but the recommended approach is to maintain a flat structure of primitives whenever possible.
A JObject should conceptually resemble a row in a database table.

```
struct Hit : public JObject {
    int x;     // Pixel coordinates
    int y;     // Pixel coordinates
    int z;     // Pixel coordinates
    double E;  // Energy loss in GeV
    double t;  // Time in us

    // Make it possible to construct a Hit as a one-liner
    Hit(int x, int y, int z, double E, double t) : x(x), y(y), z(z), E(E), t(t) {};
    ...
```

The only additional thing we need to fill out is the `Summarize` method, which aids in debugging and introspection.
Basically, it tells JANA how to convert this JObject into a (structured) string. Inside `Summarize`, we add each of 
our primitive member variables to the provided `JObjectSummary`, along with the variable name, a C-style format 
specifier, and a description of what that variable means. JANA provides a `NAME_OF` macro so that if we rename a 
member variable using automatic refactoring tools, it will automatically update the string representation of variable 
name as well. We will see where this comes in handy later.

```
    ...
    void Summarize(JObjectSummary& summary) const override {
        summary.add(x, NAME_OF(x), "%d", "Pixel coordinates centered around 0,0");
        summary.add(y, NAME_OF(y), "%d", "Pixel coordinates centered around 0,0");
        summary.add(z, NAME_OF(z), "%d", "Pixel coordinates centered around 0,0");
        summary.add(E, NAME_OF(E), "%f", "Energy loss in GeV");
        summary.add(t, NAME_OF(t), "%f", "Time in us");
    }
}
```

## Inserting JObjects into a JEvent

Now it is time to have our `RandomSource` emit events which contain `Hit` objects. For the sake
of brevity, we shall keep our hit generation logic as simple as possible: four hits which are constant.
We can make our detector simulation arbitrarily complex, but be aware that `JEventSources` only run on
a single thread by default, so complex simulations can reduce the event rate. Synchronizing `GetEvent` makes our job 
easier, however, because we can manipulate non-thread-local state such as file pointers or cursors or message buffers 
without having to worry about race conditions and deadlocks.

The pattern we use for inserting data into the event is simple: For data of type `T`, create a `std::vector<T*>`, fill
it, and pass it to `JEvent::Insert`, which will move its contents directly into the `JEvent` object. If we want,
when we insert we can also specify a tag, which is just a string. The purpose of a tag is to provide an extra level
of granularity. For instance, if we have two detectors which both use the `Hit` datatype but have separate processing
logic, we want to be able to access them independently.

```
void RandomSource::GetEvent(std::shared_ptr<JEvent> event) {
    // ...

    /// Insert simulated data into event          // ADD ME

    std::vector<Hit*> hits;                       // ADD ME
    hits.push_back(new Hit(0, 0, 0, 1.0, 0));     // ADD ME
    hits.push_back(new Hit(0, 1, 0, 1.0, 0));     // ADD ME
    hits.push_back(new Hit(1, 0, 0, 1.0, 0));     // ADD ME
    hits.push_back(new Hit(1, 1, 0, 1.0, 0));     // ADD ME
    event->Insert(hits);                          // ADD ME
    //event->Insert(hits, "fcal");                // If we used a tag
}
```

We now have `Hit`s in our event stream. The next section will cover how the `QuickTutorialProcessor` should
access them. However, we don't need to create a custom JEventProcessor to examine our event stream. JANA provides
a small utility called `JCsvWriter` which creates a CSV file containing all `JObjects` of a certain type and tag. 
It can figure out how to do this thanks to `JObject::Summarize`. You can examine the full code for `JCsvWriter` 
if you look under `$JANA_HOME/include/JANA/JCsvWriter.h`. Be aware that `JCsvWriter` is very inefficient and 
should be used for debugging, not for production.

To use `JCsvWriter`, we merely register it with our `JApplication`. If we run JANA now, 
a file 'Hit.csv' should appear in the current working directory. Note that the CSV file 
will be closed correctly even when we terminate JANA using Ctrl-C. 

```
#include<JANA/JCsvWriter.h>                       // ADD ME
#include "Hit.h"                                  // ADD ME

extern "C" {
void InitPlugin(JApplication* app) {

    InitJANAPlugin(app);

    app->Add(new QuickTutorialProcessor);
    app->Add(new RandomSource("random", app));
    app->Add(new JCsvWriter<Hit>);                // ADD ME
    //app->Add(new JCsvWriter<Hit>("fcal"));      // If we used a tag
}
```


## Retrieving JObjects from a JEventProcessor

## Creating factories

## Reading files using a JEventSource

<hr>

# Under Development

The rest of this tutorial is still under development ....

![Alt JANA Simple system with a single queue](images/queues1.png)
![Alt JANA system with multiple queues](images/queues2.png)
![Alt JANA Factory Model](images/factory_model.png)
