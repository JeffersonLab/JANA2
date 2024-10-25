
#include <JANA/Topology/JArrow.h>


void JArrow::attach(JMailbox<JEvent*>* queue, size_t port) {
    // Place index is relative to whether it is an input or not
    // Port index, however, is agnostic to whether it is an input or not
    if (port >= m_places.size()) {
        throw JException("Attempting to attach to a non-existent place! arrow=%s, port=%d", m_name.c_str(), port);
    }
    m_places[port]->is_queue = true;
    m_places[port]->place_ref = queue;
}


void JArrow::attach(JEventPool* pool, size_t port) {
    // Place index is relative to whether it is an input or not
    // Port index, however, is agnostic to whether it is an input or not
    if (port >= m_places.size()) {
        throw JException("Attempting to attach to a non-existent place! arrow=%s, port=%d", m_name.c_str(), port);
    }
    m_places[port]->is_queue = false;
    m_places[port]->place_ref = pool;
}


JEvent* JArrow::pull(size_t input_port, size_t location_id) {
    JEvent* event = nullptr;
    auto& place = m_places[input_port];
    if (place->is_queue) {
        auto queue = static_cast<JMailbox<JEvent*>*>(place->place_ref);
        queue->pop(&event, 1, 1, location_id);
    }
    else {
        auto pool = static_cast<JEventPool*>(place->place_ref);
        pool->pop(&event, 1, 1, location_id);
    }
    // If ether pop() failed, the returned event is nullptr
    return event;
}


void JArrow::push(OutputData& outputs, size_t output_count, size_t location_id) {
    for (size_t output = 0; output < output_count; ++output) {
        JEvent* event = outputs[output].first;
        int port = outputs[output].second;
        if (m_places[port]->is_queue) {
            auto queue = static_cast<JMailbox<JEvent*>*>(m_places[port]->place_ref);
            queue->push(&event, 1, location_id);
        }
        else {
            auto pool = static_cast<JEventPool*>(m_places[port]->place_ref);
            bool clear_event = !m_places[port]->is_input;
            pool->push(&event, 1, clear_event, location_id);
        }
    }
}
