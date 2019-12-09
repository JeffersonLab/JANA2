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

Our current recommendation is a `JService` called `JEventGroupTracker`. The interface looks like this:
```c++
```


1. Whenever a JEventSource emits an event, it registers that event as being part of some group.

2. The registration returns a JEventGroup pointer, which is conceptually like a coat check ticket. JEventGroup
is a persistent JObject. Insert the JEventGroup into the JEvent so that downstream components can access it.

3. In the sequential part of the JEventProcessor, call JEvent.Get<JEventGroup>()->ReportEventFinished();

4. Afterwards, the JEventProcessor may call JEvent.Get<JEventGroup>()->IsGroupFinished() to determine if that 
event was the last of its run number. This can be called from any component. For instance, if we wanted to reintroduce 
barrier events, modify the JEventSource as follows:

5. If events arrive slowly but are processed quickly, the number of in-flight events for the current run number
may become zero even though there are more events coming. To remedy this problem, JEventGroup::IsGroupFinished waits 
for an additional sign-off from the JEventSource before reporting true. From our JEventSource we call 
```c++
JEventGroupTracker::StartGroup(groupid)
JEventGroupTracker::FinishGroup(groupid)
```



