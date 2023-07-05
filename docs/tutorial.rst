Tutorial
=======

Introduction
------------

Before we begin, we need to make sure that

* The JANA library is installed
* The JANA_HOME environment variable points to the installation directory
*  Your $PATH contains $JANA_HOME/bin.

The installation process is described here. We can quickly test that our install was successful by running a builtin benchmarking/scaling test:

.. code-block:: console

    jana -Pplugins=JTest -b   # (cancel with Ctrl-C)

We can understand this command as follows:

jana is the default command-line tool for launching JANA. If you would rather create your own executable which uses JANA internally, you are free to do so.

The -P flag specifies a configuration parameter, e.g. -Pjana:debug_plugin_loading=1 tells JANA to log detailed information about where the plugin loader went looking and what it found.

plugins is the parameter specifying the names of plugins to load, as a comma-separated list (without spaces). By default JANA searches for these in $JANA_HOME/plugins, although you can also specify full paths.

-b tells JANA to run everything in benchmark mode, i.e. it slowly increases the number of threads while measuring the overall throughput. You can cancel processing at any time by pressing Ctrl-C.

Creating a JANA plugin
-----------------------

With JANA working, we can now create our own plugin. JANA provides a script which generates code skeletons to help us get started. We shall generate a skeleton for a plugin named “QuickTutorial” as follows:

.. code-block:: console

    jana-generate.py Plugin QuickTutorial

This creates the following directory tree. By default, a minimal skelton is created in a single file: QuickTutorial.cc. This provides a JEventProcessor class as well as the the plugin entry point. The generated files include lots of comments providing helpful hints on their use.

.. code-block:: console

    QuickTutorial/
    ├── CMakeLists.txt
    │├─ QuickTutorial.cc

The jana-generate.py Plugin ... command provides some option flags as well that can be given at the end of the command line. Run jana-generate.py --help to see what they are.

Integrating into an existing project
--------------------------------------

If you are working with an existing project such as eJANA or GlueX, then you don’t need the CMake project. All you need are the source files (e.g. QuickTutorial.cc):

.. code-block:: console

    cp QuickTutorial $PATH_TO_PROJECT_SOURCE/src/plugins/QuickTutorial

Be aware that you will have to manually tell the parent CMakeLists.txt to add_subdirectory(QuickTutorial).

The rest of the tutorial assumes that we are using a standalone plugin.

Building the plugin
--------------------

We build and run the plugin with the following:

.. code-block:: console

    cd QuickTutorial
    mkdir build
    cd build
    cmake3 ..
    make install
    jana -Pplugins=QuickTutorial


Adding an event source
------------------------

When we run this, we observe that JANA loads the plugin, opens our QuickTutorialProcessor, closes it again without processing any events, and exits. This is because there is nothing to do because we haven’t specified any sources. If we are running in the context of an existing project, we can pull in event sources from other plugins and observe our processor dutifully print out the event number. For now, however, we assume that we don’t have access to an event source, so we’ll create one ourselves. Our first event source will emit an infinite stream of random data, so we’ll name it RandomSource.

.. code-block:: console

    cd ..
    jana-generate.py JEventSource RandomSource

This creates two files, RandomSource.cc and RandomSource.h, in the current directory. We’ll need to add them to CMakeLists.txt ourselves. Note that we retain complete control over our directory structure. In this tutorial, for simplicity, we’ll keep all .h and .cc files in the topmost directory. For larger projects, jana-generate project MyProjectName creates a much more complex code skeleton.

To use our new RandomSource as-is, we need to do three things:

Add RandomSource.cc and RandomSource.h to the add_library(...) line in CMakeLists.txt.
Register our RandomSource with JANA inside QuickTutorial.cc
Rebuild the cmake project, rebuild the plugin target, and install.
The modified line in the CMakeLists.txt line should look like:

.. code-block:: console

    add_library(QuickTutorial_plugin SHARED QuickTutorial.cc RandomSource.cc RandomSource.h)

The modified QuickTuorial.cc file needs to have the new RandomSource.h header included so it can instantiatie an object and pass it over to the JApplication in the InitPlugin() routine. The bottom of the file should look like this:

.. code-block:: console

    #include <RandomSource.h>                             // <- ADD THIS LINE (probably better to put this at top of file)
    
    extern "C" {
        void InitPlugin(JApplication *app) {
            InitJANAPlugin(app);
            app->Add(new QuickTutorialProcessor);
            app->Add(new RandomSource("random", app));    // <- ADD THIS LINE
        }
    }

And finally, rebuild …

.. code-block:: console

    cdbuild
    make install

When we run the QuickTutorial plugin now, we observe that QuickTutorialProcessor::Process is being called on every event. Note that Process is ‘seeing’ events slightly out-of-order. This is because there are multiple threads running Process, which means that we have to be careful about how we organize the work we do inside there. This will be discussed in depth later.

Configuring an event source
----------------------------

Because neither the source nor the processor are doing any ‘real work’, the events are being processed very quickly. To throttle the rate events get emitted, to whatever frequency we like, we can add a delay inside GetEvent. Perhaps we’d even like to set the emit frequency at runtime. First, we declare a member variable on RandomSource, initializing it to our preferred default value:

.. code-block:: console

    class RandomSource : public JEventSource {
        int m_max_emit_freq_hz = 100;             // <- ADD THIS LINE

    public:
        RandomSource(std::string resource_name, JApplication* app);
        virtual ~RandomSource() = default;
        void Open() override;
        void GetEvent(std::shared_ptr<JEvent>) override;
        static std::string GetDescription();
    };

Next we sync the variable with the parameter manager inside Open. We do this by calling JApplication::SetDefaultParameter, which tells JANA to look among its configuration parameters for one called “random_source:max_emit_freq_hz”. If it finds one, it sets m_max_emit_freq_hz to the value it found. Otherwise, it leaves the variable alone. JANA remembers all such ‘default parameters’ along with their default values so that it can report them and generate config files. Note that we conventionally prefix our parameter names with the name of the requesting component or plugin. This helps prevent namespace collisions.

.. code-block:: console

    void RandomSource::Open() {
        JApplication* app = GetApplication(); 								        // <- ADD THIS LINE
        app->SetDefaultParameter("random_source:max_emit_freq_hz",            // <- ADD THIS LINE
                                 m_max_emit_freq_hz,                          // <- ADD THIS LINE
                                 "Maximum event rate [Hz] for RandomSource"); // <- ADD THIS LINE
    }

We can now use the value of m_max_emit_freq_hz, confident that it is consistent with the current runtime configuration:

.. code-block:: console

    void RandomSource::GetEvent(std::shared_ptr <JEvent> event) {

        /// Configure event and run numbers
        static size_t current_event_number = 1;
        event->SetEventNumber(current_event_number++);
        event->SetRunNumber(22);

        /// Slow down event source                                           // <- ADD THIS LINE
        auto delay_ms = std::chrono::milliseconds(1000/m_max_emit_freq_hz);  // <- ADD THIS LINE
        std::this_thread::sleep_for(delay_ms);                               // <- ADD THIS LINE
    }

Finally, we can set this parameter on the command line and observe the throughput change accordingly:

.. code-block:: console

    jana -Pplugins=QuickTutorial -Prandom_source:max_emit_freq_hz=10


Creating JObjects
------------------

So far RandomSource has been emitting events with no data attached. Now we’d like to have them emit randomly generated ‘Hit’ objects which simulate the readout from a detector. First, we need to set up our data model. Although we can insert pointers of any kind into our JEvent, we strongly recommend using JObjects for reasons we will discuss later.

.. code-block:: console

    cd src
    jana-generate.py JObject Hit


JObjects are meant to be plain-old data. For this tutorial we pretend that our detector consists of a 3D grid of sensors, each of which measures some energy at some time. Note that we are declaring Hit to be a struct instead of a class. This is because JObjects should be lightweight containers with no creation logic and no invariants which need to be encapsulated. JObjects are free to contain pointers to arbitrary data types and nested STL containers, but the recommended approach is to maintain a flat structure of primitives whenever possible. A JObject should conceptually resemble a row in a database table.

.. code-block:: console

    struct Hit : public JObject {
        int x;     // Pixel coordinates
        int y;     // Pixel coordinates
        double E;  // Energy loss in GeV
        double t;  // Time in us

        // Make it possible to construct a Hit as a one-liner
        Hit(int x, int y, double E, double t) : x(x), y(y), E(E), t(t) {};
        ...

The only additional thing we need to fill out is the Summarize method, which aids in debugging and introspection. Basically, it tells JANA how to convert this JObject into a (structured) string. Inside Summarize, we add each of our primitive member variables to the provided JObjectSummary, along with the variable name, a C-style format specifier, and a description of what that variable means. JANA provides a NAME_OF macro so that if we rename a member variable using automatic refactoring tools, it will automatically update the string representation of the variable name as well.

   .. code-block:: console

    ...
        void Summarize(JObjectSummary& summary) const override {
            summary.add(x, NAME_OF(x), "%d", "Pixel coordinates centered around 0,0");
            summary.add(y, NAME_OF(y), "%d", "Pixel coordinates centered around 0,0");
            summary.add(E, NAME_OF(E), "%f", "Energy loss in GeV");
            summary.add(t, NAME_OF(t), "%f", "Time in us");
        }
    }


Inserting JObjects into a JEvent
---------------------------------

Now it is time to have our RandomSource emit events which contain Hit objects. For the sake of brevity, we shall keep our hit generation logic as simple as possible: four hits which are constant. We can make our detector simulation arbitrarily complex, but be aware that JEventSources only run on a single thread by default, so complex simulations can reduce the event rate. Synchronizing GetEvent makes our job easier, however, because we can manipulate non-thread-local state such as file pointers or cursors or message buffers without having to worry about race conditions and deadlocks.

The pattern we use for inserting data into the event is simple: For data of type T, create a std::vector<T*>, fill it, and pass it to JEvent::Insert, which will move its contents directly into the JEvent object. If we want, when we insert we can also specify a tag, which is just a string. The purpose of a tag is to provide an extra level of granularity. For instance, if we have two detectors which both use the Hit datatype but have separate processing logic, we want to be able to access them independently.

.. code-block:: console

    #include "Hit.h"
        // ...

    void RandomSource::GetEvent(std::shared_ptr<JEvent> event) {
        // ...

        /// Insert simulated data into event       // ADD ME

        std::vector<Hit*> hits;                    // ADD ME
        hits.push_back(new Hit(0, 0, 1.0, 0));     // ADD ME
        hits.push_back(new Hit(0, 1, 1.0, 0));     // ADD ME
        hits.push_back(new Hit(1, 0, 1.0, 0));     // ADD ME
        hits.push_back(new Hit(1, 1, 1.0, 0));     // ADD ME
        event->Insert(hits);                       // ADD ME
        //event->Insert(hits, "fcal");             // If we used a tag
    }

We now have Hits in our event stream. The next section will cover how the QuickTutorialProcessor should access them. However, we don’t need to create a custom JEventProcessor to examine our event stream. JANA provides a small utility called JCsvWriter which creates a CSV file containing all JObjects of a certain type and tag. It can figure out how to do this thanks to JObject::Summarize. You can examine the full code for JCsvWriter if you look under $JANA_HOME/include/JANA/JCsvWriter.h. Be aware that JCsvWriter is very inefficient and should be used for debugging, not for production.

To use JCsvWriter, we merely register it with our JApplication. If we run JANA now, a file ‘Hit.csv’ should appear in the current working directory. Note that the CSV file will be closed correctly even when we terminate JANA using Ctrl-C.

.. code-block:: console

    #include <JANA/JCsvWriter.h>                      // ADD ME
    #include "Hit.h"                                  // ADD ME
    // ...

    extern "C" {
    void InitPlugin(JApplication* app) {

        InitJANAPlugin(app);

        app->Add(new QuickTutorialProcessor);
        app->Add(new RandomSource("random", app));
        app->Add(new JCsvWriter<Hit>);                // ADD ME
        //app->Add(new JCsvWriter<Hit>("fcal"));      // If we used a tag
    }


Writing our own JEventProcessor
--------------------------------

A JEventProcessor does two things: It calculates a bunch of intermediate results for each event (this part is done in parallel), and then it aggregates those results into a single output (this part is done sequentially). The canonical example is to calculate clusters, track candidates, and tracks separately for each event, and then produce a histogram using all of the tracks of all of the events.

In this section, we are going to modify the automatically generated TutorialProcessor to produce a heatmap that only uses hit data. We discuss how to structure more complicated calculations later. First, we add a quick-and-dirty heatmap member variable:

.. code-block:: console

    class QuickTutorialProcessor : public JEventProcessor {
        double m_heatmap[100][100];     // ADD ME
        std::mutex m_mutex;

    public:
        // ...

The heatmap itself is a piece of shared state. We have to be careful because if multiple threads try to read and write to this shared state, they will conflict with each other and corrupt it. This means we have to protect who can access it and when. Only QuickTutorialProcessor should be able to access it, so we make it a private member. However, this is not enough. Only one thread running QuickTutorialProcessor::Process must be allowed to access it at a time, which we enforce using m_mutex. Let’s look at how this is used:

.. code-block:: console

    #include "Hit.h"                                // ADD ME

    void QuickTutorialProcessor::Process(const std::shared_ptr<const JEvent> &event) {

        /// Do everything we can in parallel
        /// Warning: We are only allowed to use local variables and `event` here
        auto hits = event->Get<Hit>();              // ADD ME
    
        /// Lock mutex
        std::lock_guard<std::mutex>lock(m_mutex);

        /// Do the rest sequentially
        /// Now we are free to access shared state such as m_heatmap
        for (const Hit* hit : hits) {               // ADD ME
            m_heatmap[hit->x][hit->y] += hit->E;    // ADD ME
        }
    }

As you can see, we do everything we can in parallel, before we lock our mutex. All we are doing for now is retrieve the Hit objects we Inserted earlier, however, as we will later see, virtually all of our per-event computations will be called from here. Remember that we should only access local variables and data retrieved from a JEvent at first, whereas after we lock the mutex, we are free to access our private member variables as well.

We proceed to define our Init and Finish methods. The former zeroes out each bucket and the latter prints the heatmap to standard out as ASCII art. Note that if we want to output our results to a file all at once, we should do so in Finish. Finish will be called even if we forcibly terminate JANA with Ctrl-C. On the other hand, if we wanted to write to a file incrementally like we do with JCsvWriter, we can open it in Init, access it Process inside the lock, and close it in Finish.

.. code-block:: console

    void QuickTutorialProcessor::Init() {
        LOG << "QuickTutorialProcessor::Init: Initializing heatmap" << LOG_END;

        for (int i=0; i<100; ++i) {
            for (int j=0; j<100; ++j) {
                m_heatmap[i][j] = 0.0;
            }
        }
    }

    void QuickTutorialProcessor::Finish() {
        LOG << "QuickTutorialProcessor::Finish: Displaying heatmap" << LOG_END;

        double min_value = m_heatmap[0][0];
        double max_value = m_heatmap[0][0];

        for (int i=0; i<100; ++i) {
            for (int j=0; j<100; ++j) {
                double value = m_heatmap[i][j];
                if (min_value > value) min_value = value;
                if (max_value < value) max_value = value;
            }
        }
        if (min_value != max_value) {
            char ramp[] = " .:-=+*#%@";
            for (int i=0; i<100; ++i) {
                for (int j=0; j<100; ++j) {
                    int shade = int((m_heatmap[i][j] - min_value)/(max_value - min_value) * 9);
                    std::cout << ramp[shade];
                }
                std::cout << std::endl;
            }
        }
    }


Organizing computations using JFactories
-----------------------------------------

Just as JANA uses JObjects to organize experiment data, it uses JFactories to organize the algorithms for processing said data.

JFactories are slightly different from the ‘Factory’ design patterns: rather than abstracting away the subclass of the object being constructed, JFactories abstract away the multiplicity instead. This is a good match for nuclear and high-energy physics, where m inputs produce n outputs and n isn’t always known until after the algorithm has finished. JFactories confer other benefits as well:

Algorithms can be swapped at runtime
Results are calculated only if they are needed (‘lazy’)
Results are only calculated once and then reused as needed (‘memoized’)
JFactories are agnostic as to whether their inputs were calculated by another JFactory or inserted by a JEventSource
Different paths for deriving a result may come into play depending on the source data
For this example, we create a simple algorithm computing clusters, given hit data. We start by generating a cluster JObject:

jana-generate.py JObject Cluster

We fill out the Cluster.h skeleton, defining a cluster to be the coordinates of its center along with the total energy and time interval. Note that using JObjects helps keep our domain model malleable, so we can evolve it over time as we learn more.

.. code-block:: console

    struct Cluster : public JObject {
        double x_center;     // Pixel coordinates centered around 0,0
        double y_center;     // Pixel coordinates centered around 0,0
        double E_tot;     // Energy loss in GeV
        double t_begin;   // Time in us
        double t_end;     // Time in us

        Cluster(double x_center, double y_center, double E_tot, double t_begin, double t_end)
            : x_center(x_center), y_center(y_center), E_tot(E_tot), t_begin(t_begin), t_end(t_end) {};

        void Summarize(JObjectSummary& summary) const override {
            summary.add(x_center, NAME_OF(x_center), "%f", "Pixel coords <- [0,80)");
            summary.add(y_center, NAME_OF(y_center), "%f", "Pixel coords <- [0,24)");
            summary.add(E_tot, NAME_OF(E_tot), "%f", "Energy loss in GeV");
            summary.add(t_begin, NAME_OF(t_begin), "%f", "Earliest observed time in us");
            summary.add(t_end, NAME_OF(t_end), "%f", "Latest observed time in us");
        }
    ...
    }

Now we generate a JFactory which will compute n Clusters given m Hits. Note that we need to provide both the classname of our factory and the classname of the JObject it produces.

jana-generate.py JFactory SimpleClusterFactory Cluster

The heart of a JFactory is the function Process, where we take an event, extract whatever inputs we need by calling JEvent::Get or one of its variants, produce some number of outputs, and publish them by calling JFactory::Set. These outputs will stay cached as long as the current event is in flight and get cleared afterwards. To keep things really simple, our example shall assume there is only one cluster and all of the hits associated with this event belong to it.

.. code-block:: console

    #include "Hit.h"
    // ...

    void SimpleClusterFactory::Process(const std::shared_ptr<const JEvent> &event) {

        auto hits = event->Get<Hit>();

        auto cluster = new Cluster(0,0,0,0,0);
        for (auto hit : hits) {
            cluster->x_center += hit->x;
            cluster->y_center += hit->y;
            cluster->E_tot += hit->E;
            if (cluster->t_begin > hit->t) cluster->t_begin = hit->t;
            if (cluster->t_end < hit->t) cluster->t_end = hit->t;
        }
        cluster->x_center /= hits.size();
        cluster->y_center /= hits.size();

        std::vector<Cluster*> results;
        results.push_back(cluster);
        Set(results);
    }

For our tutorial, we don’t need to do anything inside Init or ChangeRun. Usually, these are useful for collecting statistics, or when the algorithm depends on calibration constants which we want to cache. We are free to access member variables without locking a mutex because a JFactory is assigned to at most one thread at a time.

Although JFactories are relatively simple, there are several important details. First, because each instance is assigned at most one thread, it won’t see the entire event stream. Second, there will be at least as many instances of each JFactory in existence as threads, and possibly more depending on how JANA is configured, so Initialize and ChangeRun should be fast. Thirdly, although it is tempting to use static variables to share state between different instances of the same JFactory, this practice is discouraged. That state should live in a JService instead.

Next, we register our SimpleClusterFactory with our JApplication. Because JANA will need arbitrarily many instances of these, we pass in a JFactoryGenerator which knows how to create a SimpleClusterFactory. As long as our JFactory has a zero-argument constructor, this is easy:

.. code-block:: console

    #include <JANA/JFactoryGenerator.h>                         // ADD ME
    #include "SimpleClusterFactory.h"                            // ADD ME
    // ...

    extern "C" {
    void InitPlugin(JApplication* app) {

        InitJANAPlugin(app);

        app->Add(new QuickTutorialProcessor);
        app->Add(new RandomSource("random", app));
        app->Add(new JCsvWriter<Hit>());
        app->Add(new JFactoryGeneratorT<SimpleClusterFactory>);  // ADD ME
    }
    }

We are now free to modify QuickTutorialProcessor (or create a new JEventProcessor) which histograms clusters instead of hits. Crucially, JEvent::Get doesn’t care whether the JObjects were Inserted by an event source or whether they were Set by a JFactory. The interface for retrieving them is the same either way.

Reading files using a JEventSource
-----------------------------------

Earlier we created a JEventSource which we added directly to the JApplication. This works well for simple cases but becomes cumbersome due to the amount of configuration needed: First we’d have to tell the plugin which JEventSource to register, then tell that source which files to open, and we’d have to do this for each JEventSource separately. Instead, JANA gives us a cleaner option tailored to our workflow: we specify a set of input URIs (a.k.a. file paths or sockets) and let JANA decide which JEventSource to instantiate for each. Thus we prefer to call JANA like this:

.. code-block:: console

    jana -PQuickTutorial,CsvSourcePlugin,RootSourcePlugin path/to/file1.csv path/to/file2.root

In order to make this happen, we need to define a JEventSourceGenerator. This is conceptually similar to the JFactoryGenerator we mentioned earlier, with one important addition: a method which reports back the likelihood that the underlying event source can make sense of that resource. Let’s remove the line where we added the RandomSource instance directly to the JApplication, and replace it with a corresponding JEventSourceGenerator:

.. code-block:: console

    #include <JANA/JApplication.h>
    #include <JANA/JFactoryGenerator.h>
    #include <JANA/JEventSourceGeneratorT.h>                    // ADD ME
    #include <JANA/JCsvWriter.h>

    #include "Hit.h"
    #include "RandomSource.h"
    #include "QuickTutorialProcessor.h"
    #include "SimpleClusterFactory.h"

    extern "C" {
    void InitPlugin(JApplication* app) {

        InitJANAPlugin(app);

        app->Add(new QuickTutorialProcessor);
        // app->Add(new RandomSource("random", app));           // REMOVE ME
        app->Add(new JEventSourceGeneratorT<RandomSource>);     // ADD ME
        app->Add(new JCsvWriter<Hit>());
        app->Add(new JFactoryGeneratorT<SimpleClusterFactory>);
    }
    }

By default, JEventSourceGeneratorT will report a confidence of 0.1 that it can open any resource it is given. Let’s make this more realistic: suppose we want to use this event source if and only if the resource name is “random”. In RandomSource.h, observe that jana-generate.py already declared for us:

.. code-block:: console

    template <>
    double JEventSourceGeneratorT<RandomSource>::CheckOpenable(std::string);


We fill out the definition in RandomSource.cc:

.. code-block:: console

    template <>
    double JEventSourceGeneratorT<RandomSource>::CheckOpenable(std::string resource_name) {
        return (resource_name == "random") ? 1.0 : 0.0;
    }

Note that JEventSourceGenerator puts some constraints on our JEventSource. Specifically, we need to note that:

*Our JEventSource needs a two-argument constructor which accepts a string containing the resource name, and a JApplication pointer.

*Our JEventSource needs a static method GetDescription, to help JANA report to the user which sources are available and which ended up being chosen.

*In case we need to override JANA’s preferred JEventSource for some resource, we can specify the typename of the event source we’d rather use instead via the configuration parameter event_source_type.

*When we implement Open for an event source that reads a file, we get the filename from JEventSource::GetResourceName().

Exercises for the reader
-------------------------

*Create a new JEventProcessor which generates a heatmap of Clusters instead of Hits.

*Create a BetterClusterFactory which handles multiple clusters per event. Bonus points if it is a lightweight wrapper around an industrial-strength clustering algorithm. Inside InitPlugin, use a configuration parameter to decide which JFactoryT<Cluster> gets registered with the JApplication.

*Use tags to register both ClusterFactories with the JApplication. Create a JEventProcessor which asks for the results from both algorithms and compares their results.

*Create a CsvFileSource which reads the CSV file generated from the JCsvWriter<Hit>. For CheckOpenable, read the first line of the file and check whether the column headers match what we’d expect for a table of Hits. Verify that we get the same histograms whether we use the RandomSource or the CsvFileSource.
