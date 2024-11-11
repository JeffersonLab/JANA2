
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
            if (!port.is_input) {
                event->Clear();
            }
            port.pool->Push(event, location_id);
        }
        else {
            throw JException("Arrow %s: Port %d not wired!", m_name.c_str(), port_index);
        }
    }
}

void JArrow::execute(JArrowMetrics& result, size_t location_id) {

    auto start_total_time = std::chrono::steady_clock::now();

    if (m_next_visit_time > start_total_time) {
        // If we haven't reached the next visit time, exit immediately
        result.update(JArrow::FireResult::ComeBackLater, 0, 0, std::chrono::milliseconds(0), std::chrono::milliseconds(0));
        return;
    }

    JEvent* input = nullptr;
    if (m_next_input_port != -1) {
        input = pull(m_next_input_port, location_id);
    }

    if (input == nullptr && m_next_input_port != -1) {
        // Failed to obtain the input we needed; arrow is NOT ready to fire

        auto end_total_time = std::chrono::steady_clock::now();

        result.update(JArrow::FireResult::ComeBackLater, 0, 1, std::chrono::milliseconds(0), end_total_time - start_total_time);
        return;
    }

    // Obtained the input we needed; arrow is ready to fire
    // Remember that `input` might be nullptr, in case arrow doesn't need any input event

    auto start_processing_time = std::chrono::steady_clock::now();

    OutputData outputs;
    size_t output_count;
    JArrow::FireResult status = JArrow::FireResult::KeepGoing;

    fire(input, outputs, output_count, status);

    auto end_processing_time = std::chrono::steady_clock::now();

    // Even though fire() returns an output count, some of those 'outputs' might have been sent right back to 
    // their input pool. These mustn't be counted as "processed" in the metrics.
    size_t processed_count = 0;
    for (size_t output=0; output<output_count; ++output) {
        if (!m_ports[outputs[output].second].is_input) {
            processed_count++;
        }
    }

    push(outputs, output_count, location_id);

    auto end_total_time = std::chrono::steady_clock::now();

    auto latency = (end_processing_time - start_processing_time);
    auto overhead = (end_total_time - start_total_time) - latency;
    result.update(status, processed_count, 1, latency, overhead);
}



