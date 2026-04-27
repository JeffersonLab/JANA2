
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

    class Port {
        JEventQueue* queue = nullptr;
        JEventPool* pool = nullptr;
        bool is_input = false;
        bool establishes_ordering = false;
        bool enforces_ordering = false;

    public:
        Port(){};

        bool GetEstablishesOrdering() { return establishes_ordering; }
        bool GetEnforcesOrdering() { return enforces_ordering; }
        bool GetSkipFinishEvent() { return is_input; }

        Port& SetEstablishesOrdering(bool establishes) { 
            establishes_ordering = establishes; 
            return *this;
        }

        Port& SetEnforcesOrdering(bool enforces) { 
            enforces_ordering = enforces;
            return *this;
        }

        Port& SetSkipFinishEvent(bool skip_finish_event) {
            is_input = skip_finish_event;
            return *this;
        }

        inline JEventPool* GetPool() { return pool; }
        inline JEventQueue* GetQueue() { return queue; }

        void Attach(JEventQueue* queue) {
            this->pool = nullptr;
            this->queue = queue;
        }

        void Attach(JEventPool* pool) {
            this->pool = pool;
            this->queue = nullptr;
        }
    };

private:
    std::string m_name;            // Used for human understanding
    bool m_is_parallel = false;    // Whether or not it is safe to parallelize
    bool m_is_source = false;      // Whether or not this arrow should activate/drain the topology
    bool m_is_sink = false;        // Whether or not tnis arrow contributes to the final event count

protected:
    using clock_t = std::chrono::steady_clock;

    int m_next_input_port=0; // -1 denotes "no input necessary", e.g. for barrier events
    clock_t::time_point m_next_visit_time=clock_t::now();
    std::vector<std::unique_ptr<Port>> m_ports;
    std::map<std::string, int> m_port_lookup;
    JLogger m_logger;

public:
    const std::string& get_name() { return m_name; }
    JLogger& get_logger() { return m_logger; }
    bool is_parallel() { return m_is_parallel; }
    bool is_source() { return m_is_source; }
    bool is_sink() { return m_is_sink; }
    int get_next_port_index() { return m_next_input_port; }

    void set_name(std::string name) { m_name = name; }
    void set_logger(JLogger logger) { m_logger = logger; }
    void set_is_parallel(bool is_parallel) { m_is_parallel = is_parallel; }
    void set_is_source(bool is_source) { m_is_source = is_source; }
    void set_is_sink(bool is_sink) { m_is_sink = is_sink; }

    Port& AddPort(std::string port_name);
    Port& GetPort(size_t port_index) { return *m_ports.at(port_index); }
    int GetPortIndex(const std::string& port_name) { return m_port_lookup.at(port_name); }
    void SetNextPortIndex(int input_port) { m_next_input_port = input_port; }

    JArrow() = default;
    virtual ~JArrow() = default;

    virtual void Initialize() { };

    virtual FireResult Execute(size_t location_id);

    virtual void Fire(JEvent*, OutputData&, size_t&, FireResult&) {};

    virtual void Finalize() {};

    void Attach(JEventQueue* queue, size_t port);
    void Attach(JEventPool* pool, size_t port);

    JEvent* Pull(size_t input_port, size_t location_id);
    void Push(OutputData& outputs, size_t output_count, size_t location_id);
};


std::string ToString(JArrow::FireResult r);
