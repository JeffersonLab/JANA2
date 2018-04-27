

JExample1


This example demonstates a simple event source in a plugin. To run it,
type type this:

  jana -PPLUGINS=JExample1 nada


This will run the generic jana executable and have it attach the JExample1
plugin. The "nada" argument is a dummy argument representing an event source
(usually a file name). In this case, a file is not actually read, so the 
name is not important. This will run for 1000000 "events" and then stop.

This example demonstates how to read events into JANA. No actual processing
of events is done. They are simply created, placed in the main "Physics Events"
queue, and then pulled from the queue and discarded. A number of features
that would be exercised in a standard JANA implementation are not used here
for simplicity. See other examples for those.

Details
--------------
JANA generates events by calling the GetEvent() method of a JEventSource object.
The job of the JEventSource is to read in an "event" so it can be placed on a 
queue for further processing. Here, "event" may be a complete physics event
or may be some large buffer containing a block of many physics events. In
general, the event source should only read the data with minimal processing
on it. This is because the GetEvent() calls are serialized so only one thread tries
to read from the source at a time. It is worth noting here that if multiple sources
are given to a single JANA job, it may read from those in parallel, but each one
will be read by only one thread at a time.

For simple implementations like this however, one may just want to unmarshall
the data in GetEvent(), placing the items in members of the JEvent subclass.

The JEventSourceGenerator class is what allows JANA to simultaneously support
reading files from multiple types of sources. Each source that is specified for a
job is passed as a string to the CheckOpenable() method of all 
JEventSourceGenerator objects registered in the application. Each examines
the string and returns a value from 0-1.0 indicating how confident it is
that it can read events from this source. For example, if you had two 
file formats that you wanted to read events from, their corresponding
JEventSourceGenerator classes could simply check the suffix of the source
and return 0 for all but the one type it can read. JANA will call the
MakeJEventSource method of the generator chosen to open the source.
In this example, the CheckOpenable() method always returns 0.5. It
could return anything >0 since it is the only source available in this
example and so will always be chosen.

The source for this example is compact enough that it is contained in a single
file: JExample1.cc. It defines 3 classes and the special InitPlugin()
routine that gets called when the plugin is attached. See comments in
the source for more details.


