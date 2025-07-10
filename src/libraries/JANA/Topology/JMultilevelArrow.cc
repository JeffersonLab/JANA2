
#include "JANA/Topology/JArrow.h"
#include <JANA/Topology/JMultilevelArrow.h>

void JMultilevelArrow::ConfigurePorts(Style style, std::vector<JEventLevel> levels) {
    m_levels = levels;
    m_style = style;
    size_t input_port_count = 0;
    size_t output_port_count = 0;
    if (m_style == Style::AllToAll) {
        for (auto level : levels) {
            m_port_lookup[{level, Direction::In}] = input_port_count++;
        }
        for (auto level : levels) {
            m_port_lookup[{level, Direction::Out}] = input_port_count + output_port_count++;
        }
    }
    else if (m_style == Style::AllToOne) {
        for (auto level : levels) {
            m_port_lookup[{level, Direction::In}] = input_port_count++;
        }
        output_port_count = 1;
        for (auto level : levels) {
            m_port_lookup[{level, Direction::Out}] = input_port_count;
        }
    }
    else if (m_style == Style::OneToAll) {
        input_port_count = 1;
        for (auto level : levels) {
            m_port_lookup[{level, Direction::In}] = 0;
        }
        for (auto level : levels) {
            m_port_lookup[{level, Direction::Out}] = 1 + output_port_count++;
        }
    }
    create_ports(input_port_count, output_port_count);
}

size_t JMultilevelArrow::GetPortIndex(JEventLevel level, Direction direction) {
    return m_port_lookup.at({level, direction});
}


void JMultilevelArrow::fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& fireresult) {

    // Only process new data once we've exhausted our pending outputs
    if (m_pending_outputs.size() == 0) {
        Process(input, m_pending_outputs, m_next_input_level, m_pending_fireresult);
    }

    // Put up to two outputs into the arrow's output buffer. The reason we have to paginate like this is because MultilevelArrow
    // can emit up to one JEvent for each level, whereas all previous arrows emitted <= 2 JEvents.
    if (m_pending_outputs.size() > 0) {
        auto output = m_pending_outputs.back();
        m_pending_outputs.pop_back();
        outputs[0] = {output, GetPortIndex(output->GetLevel(), Direction::Out)};
        output_count = 1;

        // See if we can grab another
        if (m_pending_outputs.size() > 0) {
            auto output = m_pending_outputs.back();
            m_pending_outputs.pop_back();
            outputs[1] = {output, GetPortIndex(output->GetLevel(), Direction::Out)};
            output_count = 2;
        }
    }

    if (m_pending_outputs.size() == 0) {
        // If we've exhausted all pending outputs, we request a new input and report the pending fire result
        fireresult = m_pending_fireresult;
        m_next_input_port = GetPortIndex(m_next_input_level, Direction::In);
    }
    else {
        // Otherwise we don't want any input, and we defer the user's fire result until all outputs have been sent
        fireresult = FireResult::KeepGoing;
        m_next_input_port = -1; // Don't want any more input data for now
    }

}


void JMultilevelArrow::Process(JEvent* input, std::vector<JEvent*>& outputs, JEventLevel& next_input_level, JArrow::FireResult& result) {

    // This is just a placeholder. A real implementation will do things like call JEventSources
    outputs.push_back(input);
    next_input_level = m_levels[0];
    result = FireResult::KeepGoing;
}


