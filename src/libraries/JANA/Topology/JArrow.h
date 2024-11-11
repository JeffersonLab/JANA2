
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <cassert>
#include <vector>

#include <JANA/JLogger.h>
#include <JANA/JException.h>
#include <JANA/Topology/JEventQueue.h>
#include <JANA/Topology/JEventPool.h>


class JArrow {
    friend class JTopologyBuilder;

public:
    using OutputData = std::array<std::pair<JEvent*, int>, 2>;
    enum class FireResult {NotRunYet, KeepGoing, ComeBackLater, Finished};

    struct Port {
        JEventQueue* queue = nullptr;
        JEventPool* pool = nullptr;
        bool is_input = false;
    };

private:
    std::string m_name;        // Used for human understanding
    bool m_is_parallel;        // Whether or not it is safe to parallelize
    bool m_is_source;          // Whether or not this arrow should activate/drain the topology
    bool m_is_sink;            // Whether or not tnis arrow contributes to the final event count

protected:
    using clock_t = std::chrono::steady_clock;

    int m_next_input_port=0; // -1 denotes "no input necessary", e.g. for barrier events
    clock_t::time_point m_next_visit_time=clock_t::now();
    std::vector<Port> m_ports;
    JLogger m_logger;

public:
    std::string get_name() { return m_name; }
    JLogger& get_logger() { return m_logger; }
    bool is_parallel() { return m_is_parallel; }
    bool is_source() { return m_is_source; }
    bool is_sink() { return m_is_sink; }
    Port& get_port(size_t port_index) { return m_ports.at(port_index); }
    int get_next_port_index() { return m_next_input_port; }

    void set_name(std::string name) { m_name = name; }
    void set_logger(JLogger logger) { m_logger = logger; }
    void set_is_parallel(bool is_parallel) { m_is_parallel = is_parallel; }
    void set_is_source(bool is_source) { m_is_source = is_source; }
    void set_is_sink(bool is_sink) { m_is_sink = is_sink; }


    JArrow() {
        m_is_parallel = false;
        m_is_source = false;
        m_is_sink = false;
    }

    JArrow(std::string name, bool is_parallel, bool is_source, bool is_sink) :
            m_name(std::move(name)), m_is_parallel(is_parallel), m_is_source(is_source), m_is_sink(is_sink) {
    };

    virtual ~JArrow() = default;

    virtual void initialize() { };

    virtual FireResult execute(size_t location_id);

    virtual void fire(JEvent*, OutputData&, size_t&, FireResult&) {};

    virtual void finalize() {};

    void create_ports(size_t inputs, size_t outputs);

    void attach(JEventQueue* queue, size_t port);
    void attach(JEventPool* pool, size_t port);

    JEvent* pull(size_t input_port, size_t location_id);
    void push(OutputData& outputs, size_t output_count, size_t location_id);
};


std::string to_string(JArrow::FireResult r);
