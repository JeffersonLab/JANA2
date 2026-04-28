
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
        std::string m_name;
        std::vector<JEventLevel> m_levels;
        JEventQueue* m_queue = nullptr;
        JEventPool* m_pool = nullptr;
        bool m_skip_finish_event = false;
        bool m_establishes_ordering = false;
        bool m_enforces_ordering = false;

    public:
        Port(std::string name, std::vector<JEventLevel> levels): m_name(name), m_levels(levels) {};

        Port(std::string name, JEventLevel level): m_name(name) {
            m_levels.push_back(level);
        };

        const std::string& GetName() { return m_name; }
        const std::vector<JEventLevel>& GetLevels() { return m_levels; }
        bool GetEstablishesOrdering() { return m_establishes_ordering; }
        bool GetEnforcesOrdering() { return m_enforces_ordering; }
        bool GetSkipFinishEvent() { return m_skip_finish_event; }

        Port& SetEstablishesOrdering(bool establishes) { 
            m_establishes_ordering = establishes; 
            return *this;
        }

        Port& SetEnforcesOrdering(bool enforces) { 
            m_enforces_ordering = enforces;
            return *this;
        }

        Port& SetSkipFinishEvent(bool skip_finish_event) {
            this->m_skip_finish_event = skip_finish_event;
            return *this;
        }

        inline JEventPool* GetPool() { return m_pool; }
        inline JEventQueue* GetQueue() { return m_queue; }

        void Attach(JEventQueue* queue) {
            this->m_pool = nullptr;
            this->m_queue = queue;
        }

        void Attach(JEventPool* pool) {
            this->m_pool = pool;
            this->m_queue = nullptr;
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
    JArrow() = default;

    virtual ~JArrow() = default;

    virtual void Initialize() {};

    virtual void Fire(JEvent*, OutputData&, size_t&, FireResult&) {};

    virtual void Finalize() {};


    FireResult Execute(size_t location_id);

    JEvent* Pull(size_t input_port, size_t location_id);

    void Push(OutputData& outputs, size_t output_count, size_t location_id);


    const std::string& GetName() { return m_name; }
    JLogger& GetLogger() { return m_logger; }
    bool IsParallel() { return m_is_parallel; }
    bool IsSource() { return m_is_source; }
    bool IsSink() { return m_is_sink; }
    int GetNextPortIndex() { return m_next_input_port; }

    void SetName(std::string name) { m_name = name; }
    void SetLogger(JLogger logger) { m_logger = logger; }
    void SetIsParallel(bool is_parallel) { m_is_parallel = is_parallel; }
    void SetIsSource(bool is_source) { m_is_source = is_source; }
    void SetIsSink(bool is_sink) { m_is_sink = is_sink; }

    Port& AddPort(std::string port_name, JEventLevel level);
    Port& AddPort(std::string port_name, std::vector<JEventLevel> levels);
    Port& GetPort(size_t port_index) { return *m_ports.at(port_index); }
    int GetPortIndex(const std::string& port_name) { 
        auto it = m_port_lookup.find(port_name);
        if (it == m_port_lookup.end()) {
            LOG_FATAL(GetLogger()) << "Unable to find port_name '" << port_name << "' on arrow '" << GetName() << "'. Valid port names are:";
            for (auto& port : m_ports) {
                LOG_FATAL(GetLogger()) << "    " << port->GetName();
            }
            throw JException("Unable to find port_name '%s' on arrow '%s'", port_name.c_str(), GetName().c_str());
        }
        return it->second;
    }
    void SetNextPortIndex(int input_port) { m_next_input_port = input_port; }

};


std::string ToString(JArrow::FireResult r);
