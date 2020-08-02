
This example demonstrates how a user can collect arbitrary statistics from their JFactories and aggregate them 
in a JEventProcessor. 

We can collect statistics from a JFactory without tying ourselves to a specific JFactory implementation by 
implementing the JMetadata template trait. Basically, each JFactory<T> produces (along with a vector of T*) 
a single struct of type JMetadata<T>, which can be defined arbitrarily as long as it is copyable.

In this example, the metadata being recorded is the latency of processing a vector of tracks. The struct
itself is defined in TrackMetadata.h. The TrackSmearingFactory will set the metadata during Process(). 
A dummy event source will also insert tracks, and set the metadata when it does so.

In its Process() method, MetadataAggregator obtains both tracks and corresponding metadata, either 
directly from the source, or from the TrackSmearingFactory. It then updates a map containing statistics 
keyed off of run number. It caches the most recently used statistics so we don't have to do a map lookup 
on each event. 

In its Finalize() method, MetadataAggregator prints statistics for each run. 

