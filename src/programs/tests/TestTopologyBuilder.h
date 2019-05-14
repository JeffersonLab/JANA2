//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_LINEARTOPOLOGY_H
#define GREENFIELD_LINEARTOPOLOGY_H

#include "TestTopology.h"
#include "SourceArrow.h"
#include "MapArrow.h"
#include "SinkArrow.h"

;
class TestTopologyBuilder {

    TestTopology& topology;
    QueueBase* last_queue;
    bool have_source = false;
    bool have_sink = false;
    bool finished = false;

public:
    TestTopologyBuilder(TestTopology& topology): topology(topology) {}

    ~TestTopologyBuilder() {}

    template <typename T>
    TestTopologyBuilder& addSource(std::string name, Source<T>& source) {

        assert(!have_sink);
        assert(!finished);
        have_source = true;
        auto output_queue = new Queue<T>;
        last_queue = output_queue;
        auto arrow = new SourceArrow<T>(name, source, output_queue);
        topology.addArrow(arrow);
        topology.addQueue(output_queue);
        return *this;
    }

    template <typename S, typename T>
    TestTopologyBuilder& addProcessor(std::string name, ParallelProcessor<S,T>& processor) {
        assert(have_source);
        assert(!have_sink);
        assert(!finished);
        auto input_queue = dynamic_cast<Queue<S>*>(last_queue);
        input_queue->set_name(name + "_queue");
        auto output_queue = new Queue<T>;
        assert(input_queue != nullptr);
        last_queue = output_queue;
        auto arrow = new MapArrow<S,T>(name, processor, input_queue, output_queue);
        topology.addArrow(arrow);
        topology.addQueue(output_queue);
        return *this;
    }

    template <typename S>
    TestTopologyBuilder& addSink(std::string name, Sink<S>& sink) {
        assert(have_source);
        assert(!have_sink);
        assert(!finished);
        have_sink = true;
        auto input_queue = dynamic_cast<Queue<S>*>(last_queue);
        input_queue->set_name(name + "_queue");
        assert(input_queue != nullptr);
        auto arrow = new SinkArrow<S>(name, sink, input_queue);
        topology.addArrow(arrow, true);
        return *this;
    }

    // "Generated" adders

    template <typename SourceT>
    TestTopologyBuilder& addSource(std::string name) {
        auto source = new SourceT;
        return addSource(name, *source);
    }

    template <typename ProcessorT>
    TestTopologyBuilder& addProcessor(std::string name) {
        auto processor = new ProcessorT;
        return addProcessor(name, *processor);
    }

    template <typename SinkT>
    TestTopologyBuilder& addSink(std::string name) {
        auto sink = new SinkT;
        return addSink(name, *sink);
    }

};

#endif //GREENFIELD_LINEARTOPOLOGY_H
