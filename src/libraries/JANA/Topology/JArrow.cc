
#include <JANA/Topology/JArrow.h>


JArrow::Port& JArrow::AddPort(std::string name) {
    if (m_port_lookup.find(name) != m_port_lookup.end()) {
        throw JException("Port with name '%s' already exists", name.c_str());
    }
    auto port = std::make_unique<Port>();
    auto port_raw_ptr = port.get();
    m_ports.push_back(std::move(port));
    m_port_lookup[name] = m_ports.size()-1;
    return *port_raw_ptr;
}

JEvent* JArrow::Pull(size_t port_index, size_t location_id) {
    JEvent* event = nullptr;
    auto& port = m_ports.at(port_index);
    if (port->GetQueue() != nullptr) {
        event = port->GetQueue()->Pop(location_id);
    }
    else if (port->GetPool() != nullptr){
        event = port->GetPool()->Pop( location_id);
    }
    else {
        throw JException("Arrow %s: Port %d not wired!", m_name.c_str(), port_index);
    }
    // If pop() failed, the returned event is nullptr
    return event;
}


void JArrow::Push(OutputData& outputs, size_t output_count, size_t location_id) {
    for (size_t output = 0; output < output_count; ++output) {
        JEvent* event = outputs[output].first;
        int port_index = outputs[output].second;
        Port& port = GetPort(port_index);
        if (port.GetQueue() != nullptr) {
            port.GetQueue()->Push(event, location_id);
        }
        else if (port.GetPool() != nullptr) {
            event->Clear(!port.GetSkipFinishEvent());
            port.GetPool()->Ingest(event, location_id);
        }
        else {
            throw JException("Arrow %s: Port %d not wired!", m_name.c_str(), port_index);
        }
    }
}

JArrow::FireResult JArrow::Execute(size_t location_id) {

    auto start_total_time = std::chrono::steady_clock::now();
    if (m_next_visit_time > start_total_time) {
        // If we haven't reached the next visit time, exit immediately
        return FireResult::ComeBackLater;
    }

    JEvent* input = nullptr;
    if (m_next_input_port != -1) {
        input = Pull(m_next_input_port, location_id);
    }

    if (input == nullptr && m_next_input_port != -1) {
        // Failed to obtain the input we needed; arrow is NOT ready to fire
        return FireResult::NotRunYet;
    }

    // Obtained the input we needed; arrow is ready to fire
    // Remember that `input` might be nullptr, in case arrow doesn't need any input event

    OutputData outputs;
    size_t output_count = 0;
    JArrow::FireResult result = JArrow::FireResult::KeepGoing;

    Fire(input, outputs, output_count, result);

    Push(outputs, output_count, location_id);

    return result;
}


std::string ToString(JArrow::FireResult r) {
    switch (r) {
        case JArrow::FireResult::NotRunYet:     return "NotRunYet";
        case JArrow::FireResult::KeepGoing:     return "KeepGoing";
        case JArrow::FireResult::ComeBackLater: return "ComeBackLater";
        case JArrow::FireResult::Finished:      return "Finished";
        default:                                return "Error";
    }
}



