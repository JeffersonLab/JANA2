//
// Created by nbrei on 3/25/19.
//

#ifndef GREENFIELD_LINEARTOPOLOGY_H
#define GREENFIELD_LINEARTOPOLOGY_H

#include <JANA/JTopology.h>
#include "Components.h"
#include "SourceArrow.h"
#include "MapArrow.h"
#include "SinkArrow.h"

class LinearTopologyBuilder {

    JTopology& topology;
    QueueBase* last_queue;
    bool have_source = false;
    bool have_sink = false;
    bool finished = false;
    std::vector<Component*> components;

public:
    LinearTopologyBuilder(JTopology& topology): topology(topology) {}

    ~LinearTopologyBuilder() {
        for (auto component : components) {
            delete component;
        }
    }

    template <typename T>
    LinearTopologyBuilder& addSource(std::string name, Source<T>& source) {

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
    LinearTopologyBuilder& addProcessor(std::string name, ParallelProcessor<S,T>& processor) {
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
    LinearTopologyBuilder& addSink(std::string name, Sink<S>& sink) {
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
    LinearTopologyBuilder& addSource(std::string name) {
        auto source = new SourceT;
        components.push_back(source);
        return addSource(name, *source);
    }

    template <typename ProcessorT>
    LinearTopologyBuilder& addProcessor(std::string name) {
        auto processor = new ProcessorT;
        components.push_back(processor);
        return addProcessor(name, *processor);
    }

    template <typename SinkT>
    LinearTopologyBuilder& addSink(std::string name) {
        auto sink = new SinkT;
        components.push_back(sink);
        return addSink(name, *sink);
    }

};

#endif //GREENFIELD_LINEARTOPOLOGY_H
