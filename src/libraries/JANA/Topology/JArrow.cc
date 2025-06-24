
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


void JArrow::attach(JEventQueue* queue, size_t port) {
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
        event = port.queue->Pop(location_id);
    }
    else if (port.pool != nullptr){
        event = port.pool->Pop( location_id);
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
            port.queue->Push(event, location_id);
        }
        else if (port.pool != nullptr) {
            event->Clear(!port.is_input);
            port.pool->Push(event, location_id);
        }
        else {
            throw JException("Arrow %s: Port %d not wired!", m_name.c_str(), port_index);
        }
    }
}

JArrow::FireResult JArrow::execute(size_t location_id) {

    auto start_total_time = std::chrono::steady_clock::now();
    if (m_next_visit_time > start_total_time) {
        // If we haven't reached the next visit time, exit immediately
        return FireResult::ComeBackLater;
    }

    JEvent* input = nullptr;
    if (m_next_input_port != -1) {
        input = pull(m_next_input_port, location_id);
    }

    if (input == nullptr && m_next_input_port != -1) {
        // Failed to obtain the input we needed; arrow is NOT ready to fire
        return FireResult::NotRunYet;
    }

    // Obtained the input we needed; arrow is ready to fire
    // Remember that `input` might be nullptr, in case arrow doesn't need any input event

    OutputData outputs;
    size_t output_count;
    JArrow::FireResult result = JArrow::FireResult::KeepGoing;

    fire(input, outputs, output_count, result);

    push(outputs, output_count, location_id);

    return result;
}


std::string to_string(JArrow::FireResult r) {
    switch (r) {
        case JArrow::FireResult::NotRunYet:     return "NotRunYet";
        case JArrow::FireResult::KeepGoing:     return "KeepGoing";
        case JArrow::FireResult::ComeBackLater: return "ComeBackLater";
        case JArrow::FireResult::Finished:      return "Finished";
        default:                                return "Error";
    }
}



