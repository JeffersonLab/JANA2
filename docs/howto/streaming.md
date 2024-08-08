# Stream data to and from JANA


## Messages and Events

1. The first question to ask is: What is the relationship between messages and events? Remember, a message is just 
   a packet of data sent over the wire, whereas an event is JANA's main unit of independent computation, corresponding
   to all data associated with one physics interaction. The answer will depend on:
   
   - What systems already exist upstream, and how difficult they are to change
   - The expected size of each event
   - Whether event building is handled upstream or within JANA
   
   If events are large enough (>0.5MB), the cleanest thing to do is to 
   establish a one-to-one relationship between messages and events. JANA provides 
   [JStreamingEventSource](html/class_j_streaming_event_source.html)
   to make this convenient.
     
   If events are very small, you probably want many events in one message. A corresponding helper class does not 
   exist yet, but would be a straightforward adaptation of the above.
   
   If upstream doesn't do any event building (e.g. it is reading out ADC samples over a fixed time window) you 
   probably want to have JANA determine physically meaningful event boundaries, maybe even incorporating a software 
   L2 trigger. This is considerably more complicated, and is discussed in [the event building how-to](Howto.html) 
   instead.
      
   For the remainder of this how-to we assume that messages and events are one-to-one.


## Transport

2. The second question to ask is: What transport should be used? 

    JANA makes it so that the message format and transport can be varied independently. The transport wrapper need only
    implement the [JTransport](html/struct_j_transport.html) interface, which is essentially just:

    ```c++
        enum class Result {SUCCESS, TRYAGAIN};
        
        virtual void initialize();
        virtual Result send(const JMessage& src_msg);
        virtual Result receive(JMessage& dest_msg);
    ```

    The key detail is that both `send` and `receive` should block until data has finished transferring to/from the `JMessage`
    buffer so that the buffer may be accessed by the caller with no additional synchronization. If there are no pending 
     messages, `receive` should return `TRYAGAIN` immediately so as not to block the event source. In contrast, 
     `send` must block until it succeeds, as otherwise there will be data loss.

    An implementation already exists for ZeroMQ. See `examples/JExample7/ZmqTransport.h` 

## Message format

3. The final and most important question to ask is: What is the message format?

    Message formats each get their own class, which must inherit from the JMessage and JEventMessage interfaces.

4. 
   


