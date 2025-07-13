#include "JANA/Topology/JArrow.h"
#include <JANA/Topology/JMultilevelSourceArrow.h>


const std::vector<JEventLevel>& JMultilevelSourceArrow::GetLevels() const {
    return m_levels;
}

size_t JMultilevelSourceArrow::GetPortIndex(JEventLevel level, Direction direction) const {
    return m_port_lookup.at({level, direction});
};

 void JMultilevelSourceArrow::SetLevels(std::vector<JEventLevel> levels) {
    m_levels = levels;
    m_child_event_level = levels.back();
    m_next_input_port = 0;

    size_t input_port_count = 0;
    size_t output_port_count = 0;
    for (auto level : levels) {
        m_port_lookup[{level, Direction::In}] = input_port_count++;
    }
    for (auto level : levels) {
        m_port_lookup[{level, Direction::Out}] = input_port_count + output_port_count++;
    }

    create_ports(input_port_count, output_port_count);
}

void JMultilevelSourceArrow::initialize() {}

void JMultilevelSourceArrow::finalize() {}

JEventSource::Result JMultilevelSourceArrow::DoNext(JEvent& event) {
    // Cycle through each of the input levels one after another
    event.SetEventNumber(m_total_emitted_event_count);
    m_total_emitted_event_count += 1; // Avoid ever requesting 2 of the same event level in a row
    m_next_input_level = GetLevels().at(m_total_emitted_event_count % GetLevels().size()); // Switch to the next event level next time
    LOG_INFO(get_logger()) << "Producing " << toString(event.GetLevel()) << " number " << event.GetEventNumber() << ". Next input level=" << toString(m_next_input_level);

    if (m_total_emitted_event_count > 9) {
        return JEventSource::Result::FailureFinished;
    }
    else {
        return JEventSource::Result::Success;
    }
    return JEventSource::Result::Success;
}
void JMultilevelSourceArrow::EvictParent(JEventLevel level, OutputData& outputs) {

}

void JMultilevelSourceArrow::fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) {

    if (!m_finish_in_progress) {

        auto result = DoNext(*input);

        // First we have to consider whether the user changed m_next_input_level, regardless of result returned
        // If so, this puts an evicted parent in the output buffer

        m_next_input_port = GetPortIndex(m_next_input_level, Direction::In);
        LOG_INFO(get_logger()) << "Changing input port to " << m_next_input_port << " because level is now " << toString(m_next_input_level);

        // This is a little bit tricky: Ideally we would be able to constrain max_inflight_events:$PARENT_LEVEL to be 1, 
        // which would essentially behave like (nicer) barrier events. However, if there is only 1 event
        // inflight at $PARENT_LEVEL, we have to evict immediately so that we don't deadlock.

        auto it = m_pending_parents.find(m_next_input_level);
        if (it != m_pending_parents.end()) {
            if (it->second.first != nullptr) {
                // There IS an old parent
                size_t parent_output_port = GetPortIndex(m_next_input_level, Direction::Out);
                LOG_INFO(get_logger()) << "Evicting parent " << toString(m_next_input_level) << " to port " << parent_output_port;
                outputs.at(0) = {it->second.first, parent_output_port};
                it->second.first = nullptr;
                output_count = 1;
            }
        }


        if (result == JEventSource::Result::Success) {
            // We have a newly filled event we have to do something with

            if (input->GetLevel() == m_child_event_level) {
                // We acquired a child! Attach it to its parents and push it into the big wide world

                for (auto [level, parent_pair] : m_pending_parents) {
                    // Note that this only attaches parents that we already, so if the parents arrive in the wrong order they
                    // will just be missing. If this is expected behavior, you'll need to set your downstream parent inputs to be optional.
                    if (parent_pair.first != nullptr) {
                        LOG_INFO(get_logger()) << "Attaching parent: " << toString(level) << " with number " << parent_pair.first->GetEventNumber() << " to event " << toString(input->GetLevel()) << " " << input->GetEventNumber();
                        input->SetParent(parent_pair.first);
                    }
                }
                outputs.at(output_count++) = {input, GetPortIndex(m_child_event_level, Direction::Out)};
                status = JArrow::FireResult::KeepGoing;
                return;
            }
            else {
                // We acquired a parent event! It's predecessor SHOULD have already been evicted during the previous call to fire()

                auto it = m_pending_parents.find(input->GetLevel());
                if (it != m_pending_parents.end()) {
                    // There IS an old parent
                    if (it->second.first != nullptr) { 
                        throw JException("Found a parent event we weren't expecting"); 
                    }
                    it->second.first = input;
                    it->second.second += 1; // Increment parent count
                    status = JArrow::FireResult::KeepGoing;
                    return;
                }
                else {
                    m_pending_parents[input->GetLevel()] = {input, 1};
                    return;
                }
            }
        }
        else if (result == JEventSource::Result::FailureTryAgain) {
            // Return this event to the pool with no further action
            outputs.at(output_count++) = {input, GetPortIndex(input->GetLevel(), Direction::In)};
            status = JArrow::FireResult::ComeBackLater;
            return;
        }
        else if (result == JEventSource::Result::FailureLevelChange) {
            // Return this input event to the pool
            outputs.at(output_count++) = {input, GetPortIndex(input->GetLevel(), Direction::In)};

            status = JArrow::FireResult::KeepGoing;
            return;
        }
        else if (result == JEventSource::Result::FailureFinished) {
            // Return this input event to the pool
            outputs.at(output_count++) = {input, GetPortIndex(input->GetLevel(), Direction::In)};
            m_finish_in_progress = true;
            // Fall-through to if (finish_in_progress) below
        }
    }
    // At this point the only thing left to do is to evict ALL parents using as many fire() calls as necessary

    bool no_more_parents = false;
    while (output_count < 2 && !no_more_parents) {
        // We improvise our own m_pending_parents.pop()
        auto it = m_pending_parents.begin();
        if (it != m_pending_parents.end()) {
            // Found a parent
            auto parent = it->second.first;
            if (parent != nullptr) {
                outputs.at(output_count++) = {parent, GetPortIndex(parent->GetLevel(), Direction::Out)};
            }
            m_pending_parents.erase(it);
        }
        else {
            no_more_parents = true;
        }
    }
    status = (no_more_parents) ? JArrow::FireResult::Finished : JArrow::FireResult::KeepGoing;
    return;
}
