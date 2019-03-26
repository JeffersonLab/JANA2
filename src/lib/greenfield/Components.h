//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_COMPONENTS_H
#define GREENFIELD_COMPONENTS_H

#include <vector>
#include <cstddef>

namespace greenfield {

/// Abstract classes for very generic components that the user is expected to furnish.
/// They do not contain any code specific to any parallelization paradigm whatsoever.
/// This is the most common interaction the user should have with the JANA API.
/// We can think of these as a stand-in for JEventSource and JEventProcessor, and
/// thereby worry about evolving them later.

/// There are two main benefits of structuring our API like this:
/// 1. We can change a lot of our internal code without breaking user code
/// 2. Our users can test and debug their components easily: they don't have to worry about
///    initializing secret global state, or launching mock JApplications, or even running
///    on multiple threads at first. The only thing they will need to mock is a JEvent.


/// Common base class for all components
struct Component {};


/// Return codes for Sources. These are identical to Queue::StreamStatus (on purpose, because
/// it is convenient to model a Source as a stream) but we keep them separate because one is part
/// of the API and the other is an internal implementation detail.

enum class SourceStatus {KeepGoing, ComeBackLater, Finished, Error};


/// Source stands in for (and improves upon) JEventSource.
/// The type signature of inprocess() is chosen with the following goals:
///    - Chunking data in order to increase parallelism coarseness
///    - The 'no more events' condition is handled automatically
///    - Status information such as 'error' or 'finished' is decoupled from the actual stream of Events
///    - The user is given the opportunity to implement their own rate-limiting

template <typename T>
struct Source : Component {
    virtual void initialize() = 0;
    virtual void finalize() = 0;
    virtual SourceStatus inprocess(std::vector<T>& ts, size_t count) = 0;
};


/// Sink consumes events of type T, accumulating state along the way. This
/// state is supposed to reside on the Sink itself.
/// The final result of this accumulation is a side-effect which should be
/// safe to retrieve after finalize() is called. (finalize() will be called
/// automatically after all upstream events have been processed)
/// This is conceptually equivalent to the part of JEventProcessor::Process
/// after the lock is acquired.

template <typename T>
struct Sink : Component {
    virtual void initialize() = 0;
    virtual void finalize() = 0;
    virtual void outprocess(T t) = 0;
};


/// ParallelProcessor transforms S to T and it does so in a way which is thread-safe
/// and ideally stateless. It is conceptually equivalent to the first part
/// of JEventProcessor::Process, i.e. up until the lock is acquired. Alternatively, it could
/// become a JFactorySet, in which case process() would call all Factories present, thereby
/// making sure that everything which can be calculated in parallel has in fact been, before
/// proceeding to the (sequential) Sink.

template <typename S, typename T>
struct ParallelProcessor : Component {
    virtual T process(S s) = 0;
};


/// SequentialProcessor transforms a stream in a very general way, consuming some
/// number of input messages, emitting some number of output messages, and probably
/// storing state along the way. Not parallelizable. If we maintained a queue of
/// recycled JEvents, then we might implement both Sources and Sinks as SequentialProcessors
/// instead.

template <typename S, typename T>
struct SequentialProcessor : Component {

    virtual void initialize() = 0;
    virtual void finalize() = 0;
    virtual void process(std::vector<S>& inputs, std::vector<T>& outputs) = 0;
};


/// SubtaskProcessor offers sub-event-level parallelism. The idea is to split parent
/// event S into independent subtasks T, and automatically bundling them with
/// bookkeeping information X onto a Queue<pair<T,X>. process :: T -> U handles the stateless,
/// parallel parts; its Arrow pushes messages on to a Queue<pair<U,X>, so that merge() :: S -> [U] -> V
/// "joins" all completed "subtasks" of type U corresponding to one parent of type S, (i.e.
/// a specific JEvent), back into a single entity of type V, (most likely the same JEvent as S,
/// only now containing more data) which is pushed onto a Queue<V>, bookkeeping information now gone.
/// Note that there is no blocking and that our streaming paradigm is not compromised.
/// If we want multiple levels of Subtasks, we can do that too, but we would probably
/// want to split this into three separate classes (one for each arrow).

template <typename S, typename T, typename U, typename V>
struct SubtaskProcessor : Component {

    virtual void split(S s, std::vector<T>& ts) = 0;
    virtual U process(T& t) = 0;
    virtual void merge(S parent, std::vector<U>& inputs, std::vector<V>& outputs) = 0;
};


} // namespace greenfield

#endif //GREENFIELD_COMPONENTS_H
