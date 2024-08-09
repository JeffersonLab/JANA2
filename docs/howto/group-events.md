
# Group events <!-- {docsify-ignore-all} -->

## Detect when a group of events has finished

Sometimes it is necessary to organize events into groups, process the events the usual way, but then notify 
some component whenever a group has completely finished. The original motivating example for this was EPICS data, 
which was maintained as a bundle of shared state. Whenever updates arrived, JANA1 would emit a 'barrier event' 
which would stop the data flow until all in-flight events completed, so that preceding events could only read the old 
state and subsequent events could only read the new state. [We now recommend EPICS data be handled differently.](howto_epicsdata) 
Nevertheless this pattern still occasionally comes into play. 

One example is a JEventProcessor which writes statistics for the previous run every time the run number changes. 
This is trickier than it first appears because events may arrive out of order. The JEventProcessor can easily maintain 
a set of run numbers it has already seen, but it won't know when it has seen all of the events for a given run number. 
For that it needs an additional piece of information: the number of events emitted with that run number.
Complicating things further, this information needs to be read and modified by both the JEventSource and the JEventProcessor.

Our current recommendation is a `JService` called `JEventGroupManager`. This is designed to be used as follows:

1. A JEventSource should keep a pointer to the current JEventGroup, which it obtains through the JEventGroupManager.
Groups are given a unique id, which 

2. Whenever the JEventSource emits a new event, it should insert the JEventGroup into the JEvent. The event is
now tagged as belonging to that group.

3. When the JEventSource moves on to the next group, e.g. if the run number changed, it should close out the old group
by calling JEventGroup::CloseGroup(). The group needs to be closed before it will report itself as finished, even 
if there are no events still in-flight.

4. A JEventProcessor should retrieve the JEventGroup object by calling JEvent::Get. It should report that an event is
finished by calling JEventGroup::FinishEvent. Please only call this once; although we could make JEventGroup robust 
against repeated calls, it would add some overhead.

5. A JEventSource or JEventProcessor (or technically anything whose lifespan is enclosed by the lifespan of JServices) 
may then test whether this is the last event in its group by calling JEventGroup::IsGroupFinished(). A blocking version, 
JEventGroup::WaitUntilGroupFinished(), is also provided. This mechanism allows relatively arbitrary hooks into the 
event stream.



