

JThread
---------

The heart of the framework is in the JThread objects and the heart of them is in the Loop method.
Here is a basic description of what goes on in Loop:

The loop is controlled by a member variable "mRunStateTarget". This can be set externally, primarily
to tell the loop to pause or quit at the next iteration without interferening with what it is currently 
doing. Valid values are defined in the RUN_STATE_t enum defined inside the JThread class.

The basic workflow in one loop's iteration is:

1. Check if events need to be read from the source
2. Get the next task from the current event source's JQueueSet and run it if there is one
3. If there is no task then call the HandleNullTask method. This will:
   a. Check if all tasks and sub-tasks from the current source are done and if so,
       inform the JThreadManager that the current source is exhausted and we need 
       a new one
   b. If the current source is not completed, but there are currently no tasks available
       from it for the thread to work on, try switching to another open source and getting
       tasks from it. (This only happens if multiple sources are open simultaneously.)

For item 1. the Loop may actually jump back to the beginning as it reads in events without processing
them in an effort to make reading from the source more efficient. The idea is a quick succession of
read requests will keep the disk spinning and the head from thrashing so you gain overall by buffering
several events at once. How many are buffered is controlled by the the JEventSource (see
SetNumEventsToGetAtOnce method.) This feature may not matter for SSD's or reads.

A JThread object accesses a JEventSource via the JThreadManager::JEventSourceInfo class.
This class is used to group a JEventSource pointer and a JQueueSet pointer together as a
single entity. Note that the JEventSource will provide a single JQueue for its events. The 
JQueueSet is created by the JThreadManager and adds the source's queue as well as any
queue's from the base set of queue's in the mTemplateQueueSet member of the JThreadManager.
The JQueue objects in the set are clones of the ones found in mTemplateQueueSet so they
form a set unique to the JEventSource.


JThreadManager
--------------------

A single JThreadManager object exists for the process. It is created in the JApplication constructor
and is owned by the JApplication object. It is responsible for creating and managing threads as
well as their associated JQueueSet objects. It also manages which JEventSource object a JThread
object uses at any given time.


JQueueSet
--------------------

Each JEventSource will have it's own JQueueSet object created for it. The JQueueSet object is
created and owned by the JThreadManager. This is because it must combine the JQueue object
from the JEventSource itself with any other JQueue objects coming from reconstruction
code. Keeping a separate JQueueSet for each source allows events from that source to be 
processed independently from other sources. Specifically, barrier events only need to serialize
the processing of data from the source they came from. Since JANA supports reading from
multiple JEventSource objects simultaneously, unique JQueueSet objects are needed.

The JQueueSet is queried for a task from the JThread::Loop. The JQueueSet will loop
through it's queue's by type first and then JQueue objects of that type to ensure a priority
for the order in which tasks are processed. The JQueueSet keeps the JQueue objects in a
std::map with keys being the queue type (defined in an enum that is part of the JQueueSet
class). The values of the map are std::vector containers holding the actual queues. Due to the
std::map ordering, the priority is given to the queue type in the order they are defined in the
enum. Specifically: Output, SubTasks, Events. Thus, if an Event task is performed that creates
subtasks, then those will be performed next before another task is pulled from an Event queue.

JQueueSet objects are made by combining the JQueue object from a JEventSource and
clones of any JQueue objects in the JThreadManager::mTemplateQueueSet object. Plugins
and libraries will need to add their JQueue objects to the template queue set by calling
the AddQueue method of JThreadManager before event processing starts. Note that
the JQueue object passed into AddQueue will never actually be used to hold any tasks.
It will just be used to clone itself to make new JQueue objects to go into JQueueSet objects
as needed.


JEventSource
--------------------

JEventSource is a base class for classes that read events into the program. User's
will need to provide a class that inherits from this in order to provide input events
to the JANA program.

## GetEvent:
This method will be called to read in one "event" from the source. The
event may simply be a large buffer containing one or more physics events. This is
called serially so generally very little data processing should be done during this
call. See the GetProcessEventTask method for details on how to setup the parsing
as a separate that can be done in parallel while other events are being parsed or
processed.

## GetObjects: 
This method will be called when a factory or processor requests some
objects for the event. The JEvent passed into this will be of the form that has the
objects available. i.e. For more complicated sources that implement multiple
queues to hold different forms of the event as it is parsed, this will typically be the
final form of the event.

## GetProcessEventTask:
This method is called to make a JTask object for a given JEvent.
The JEvent will have been obtained by an earlier call to GetEvent(). This is where one
would implement code to process the JEvent. For example, the GetEvent method 
might only read in an event buffer and wrap it as a JEvent object. Any parsing of the
buffer is deferred so it can be done in parallel as opposed to parsing it in the GetEvent
method which is called serially. The parsing code would naturally fit into the 
process event task.

There is a default GetProcessEventTask method defined in JEventSource that will make
a JTask that simply calls all of the JEventProcessors for the JEvent. (This uses the
JMakeAnalyzeEventTask routine defined in JFunctions.cc.) If the user provides their
own GetProcessEventTask method, they should eventually generate a task that 
runs the processors. 


 













