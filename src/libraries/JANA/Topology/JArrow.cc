
#include <JANA/Topology/JArrow.h>

void JArrow::create_ports(size_t inputs, size_t outputs) {
    m_ports.clear();
    for (size_t i=0; i<inputs; ++i) {
        m_ports.push_back({nullptr, nullptr, true});
    }
    for (size_t i=0; i<outputs; ++i) {
        m_ports.push_back({nullptr, nullptr, false});
    }
}


void JArrow::attach(JMailbox<JEvent*>* queue, size_t port) {
    // Place index is relative to whether it is an input or not
    // Port index, however, is agnostic to whether it is an input or not
    if (port >= m_ports.size()) {
        throw JException("Attempting to attach to a non-existent port! arrow=%s, port=%d", m_name.c_str(), port);
    }
    m_ports[port].queue = queue;
}


void JArrow::attach(JEventPool* pool, size_t port) {
    // Place index is relative to whether it is an input or not
    // Port index, however, is agnostic to whether it is an input or not
    if (port >= m_ports.size()) {
        throw JException("Attempting to attach to a non-existent place! arrow=%s, port=%d", m_name.c_str(), port);
    }
    m_ports[port].pool = pool;
}


JEvent* JArrow::pull(size_t port_index, size_t location_id) {
    JEvent* event = nullptr;
    auto& port = m_ports.at(port_index);
    if (port.queue != nullptr) {
        port.queue->pop(&event, 1, 1, location_id);
    }
    else if (port.pool != nullptr){
        port.pool->pop(&event, 1, 1, location_id);
    }
    else {
        throw JException("Arrow %s: Port %d not wired!", m_name.c_str(), port_index);
    }
    // If pop() failed, the returned event is nullptr
    return event;
}


void JArrow::push(OutputData& outputs, size_t output_count, size_t location_id) {
    for (size_t output = 0; output < output_count; ++output) {
        JEvent* event = outputs[output].first;
        int port_index = outputs[output].second;
        Port& port = m_ports.at(port_index);
        if (port.queue != nullptr) {
            port.queue->push(&event, 1, location_id);
        }
        else if (port.pool != nullptr) {
            bool clear_event = !port.is_input;
            port.pool->push(&event, 1, clear_event, location_id);
        }
        else {
            throw JException("Arrow %s: Port %d not wired!", m_name.c_str(), port_index);
        }
    }
}

inline size_t JArrow::get_pending() { 
    size_t sum = 0;
    for (Port& port : m_ports) {
        if (port.is_input && port.queue != nullptr) {
            sum += port.queue->size();
        }
    }
    return sum;
}
