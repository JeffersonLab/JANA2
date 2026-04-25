
#include <iostream>

#include <JANA/Topology/JEventPool.h>
#include <JANA/Topology/JEventQueue.h>

class Arrow {
public:

    struct Port {
        JEventQueue* queue = nullptr;
        JEventPool* pool = nullptr;
        bool is_input = false;
        bool establishes_ordering = false;
        bool enforces_ordering = false;
    };

protected:
    std::string m_name;
    bool m_is_parallel = false;
    bool m_is_source = false;
    bool m_is_sink = false;

private:
    using clock_t = std::chrono::steady_clock;
    clock_t::time_point m_next_visit_time = clock_t::now();
    std::vector<Port> m_ports;
    int m_next_input_port = 0; // -1 denotes "no input necessary", e.g. for barrier events

public:
    const std::string& GetName() { return m_name; }
    bool IsParallel() { return m_is_parallel; }
    bool IsSource() { return m_is_source; }
    bool IsSink() { return m_is_sink; }

    int GetNextInputPort() { return m_next_input_port; }
    virtual void Fire()


};
