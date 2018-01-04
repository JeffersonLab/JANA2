

JExample1


This example demonstates a simple event source in a plugin. To run it,
type type this:

  jana -PPLUGINS=JExample1 nada


This will run the generic jana executable and have it attach the JExample1
plugin. The "nada" argument is a dummy argument representing an event source
(usually a file name). In this case, a file is not actually read, so the 
name is not important. This will run continuously. Hit ctl-C when you wish
to stop it.

This example demonstates how to read events into JANA. No actual processing
of events is done. They are simply created, placed in the main "Physics Events"
queue, and then pulled from the queue and discarded. A number of features
that would be exercised in a standard JANA implementation are not used here
for simplicity. See other examples for those.

Details
--------------
JANA generates events by calling the GetEvent() method of a JEventSource object.
More specifically, an object whose class inherits from JEventSource. The job
of the JEventSource is to add JEvent objects it creates to an appropriate JQueue
(usually from data it has read from a file). The exact data held by the JEvent
is up to the user. In this example, JEvent_example1 has two members: "A" and "B".
Data read from the source (e.g. file) can be stored in members like this.
Alternatively, the JEvent base class also has features built in to help store
and retrieve objects of type JObject. Those features are not used in this
example.

The JEventSourceGenerator class is what allows JANA to simultaneously support
reading files from multiple types of sources. Each source specified when a
job is run is passed as a string to the CheckOpenable() method of all 
JEventSourceGenerator objects registered in the application. Each examines
the string and returns a value from 0-1 indicating how confident it is
that it can read events from this source. For example, if you had two 
file formats that you wanted to read events from, their corresponding
JEventSourceGenerator classes could simply check the suffix of the source
and return 0 for all but the one type it can read. JANA will call the
MakeJEventSource method of the generator chosen to open the source.
In this example, the CheckOpenable() method always returns 0.5. It
could return anything >0 since it is the only source available in this
example and so will always be chosen.


The source for this is compact enough that it is contained in a single
file: JExample1.cc. It defines 3 classes and the special InitPlugin()
routine that gets called when the plugin is attached. See comments in
the source for details.


