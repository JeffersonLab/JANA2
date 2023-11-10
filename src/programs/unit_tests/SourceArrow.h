
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef GREENFIELD_SOURCEARROW_H
#define GREENFIELD_SOURCEARROW_H

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>


/// Source stands in for (and improves upon) JEventSource.
/// The type signature of inprocess() is chosen with the following goals:
///    - Chunking data in order to increase parallelism coarseness
///    - The 'no more events' condition is handled automatically
///    - Status information such as 'error' or 'finished' is decoupled from the actual stream of Events
///    - The user is given the opportunity to implement their own rate-limiting
template <typename T>
struct Source {

    /// Return codes for Sources. These are identical to Queue::StreamStatus (on purpose, because
    /// it is convenient to model a Source as a stream) but we keep them separate because one is part
    /// of the API and the other is an internal implementation detail.
    enum class Status {KeepGoing, ComeBackLater, Finished, Error};

    virtual void initialize() = 0;
    virtual void finalize() = 0;
    virtual Status inprocess(std::vector<T>& ts, size_t count) = 0;
};


/// SourceArrow wraps a reference to a Source and 'lifts' it into a Arrow
/// that knows how to stream to and from queues, and is executable by a Worker
template<typename T>
class SourceArrow : public JArrow {

private:
    Source<T> & _source;
    JMailbox<T> * _output_queue;
    std::vector<T> _chunk_buffer;


public:
    SourceArrow(std::string name, Source<T>& source, JMailbox<T> *output_queue)
        : JArrow(name, false, NodeType::Source)
        , _source(source)
        , _output_queue(output_queue) {
    }

    void execute(JArrowMetrics& result, size_t /* location_id */) override {

        auto start_time = std::chrono::steady_clock::now();
        auto in_status = _source.inprocess(_chunk_buffer, get_chunksize());
        auto latency_time = std::chrono::steady_clock::now();
        auto out_status = _output_queue->push(_chunk_buffer);
        auto message_count = _chunk_buffer.size();
        auto finished_time = std::chrono::steady_clock::now();

        auto latency = (latency_time - start_time);
        auto overhead = (finished_time - latency_time);

        JArrowMetrics::Status status;
        if (in_status == Source<T>::Status::Finished) {
            status = JArrowMetrics::Status::Finished;
        }
        else if (in_status == Source<T>::Status::KeepGoing && out_status == JMailbox<T>::Status::Ready) {
            status = JArrowMetrics::Status::KeepGoing;
        }
        else {
            status = JArrowMetrics::Status::ComeBackLater;
        }
        result.update(status, message_count, 1, latency, overhead);
    }


    void initialize() override {
        _source.initialize();
    }

    void finalize() override {
        _source.finalize();
    }
};


#endif //GREENFIELD_SOURCEARROW_H
