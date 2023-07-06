how-to instructions
====================
Stream data to and from JANA
--------------------------------
1. The first question to ask is: What is the relationship between messages and events? Remember, a message is just a packet of data sent over the wire, whereas an event is JANA’s main unit of independent computation, corresponding to all data associated with one physics interaction. The answer will depend on:

* What systems already exist upstream, and how difficult they are to change
* The expected size of each event
* Whether event building is handled upstream or within JANA

If events are large enough (>0.5MB), the cleanest thing to do is to establish a one-to-one relationship between messages and events. JANA provides JStreamingEventSource to make this convenient.

If events are very small, you probably want many events in one message. A corresponding helper class does not exist yet, but would be a straightforward adaptation of the above.

If upstream doesn’t do any event building (e.g. it is reading out ADC samples over a fixed time window) you probably want to have JANA determine physically meaningful event boundaries, maybe even incorporating a software L2 trigger. This is considerably more complicated, and is discussed in `the event building how-to <https://jana.readthedocs.io/en/latest/how-to%20guides.html#>`_ instead.

For the remainder of this how-to we assume that messages and events are one-to-one.
