#include "JANA/JEventSource.h"
#include "JANA/Topology/JArrow.h"
#include "JANA/Utils/JEventLevel.h"
#include <JANA/Topology/JMultilevelSourceArrow.h>


void JMultilevelSourceArrow::SetEventSource(JEventSource* source) {
    m_source = source;
    m_levels = source->GetEventLevels();
    m_child_event_level = m_levels.back();
    m_next_input_port = 0;

    size_t input_port_count = 0;
    size_t output_port_count = 0;
    for (auto level : m_levels) {
        m_port_lookup[{level, Direction::In}] = input_port_count++;
    }
    for (auto level : m_levels) {
        m_port_lookup[{level, Direction::Out}] = input_port_count + output_port_count++;
    }

    create_ports(input_port_count, output_port_count);
}

const std::vector<JEventLevel>& JMultilevelSourceArrow::GetLevels() const {
    return m_levels;
}

size_t JMultilevelSourceArrow::GetPortIndex(JEventLevel level, Direction direction) const {
    return m_port_lookup.at({level, direction});
};

void JMultilevelSourceArrow::initialize() {
    // We initialize everything immediately, but don't open any resources until we absolutely have to; see process(): source->DoNext()
    m_source->DoInit();
}

void JMultilevelSourceArrow::finalize() {
    // Generally JEventSources finalize themselves as soon as they detect that they have run out of events.
    // However, we can't rely on the JEventSources turning themselves off since execution can be externally paused.
    // Instead we leave everything open until we finalize the whole topology, and finalize remaining event sources then.
    m_source->DoClose();
}

void JMultilevelSourceArrow::EvictNextParent(OutputData& outputs, size_t& output_count) {
    // This is a little bit tricky: Ideally we would be able to constrain max_inflight_events:$PARENT_LEVEL to be 1, 
    // which would essentially behave like (nicer) barrier events. However, if there is only 1 event
    // inflight at $PARENT_LEVEL, we have to evict immediately so that we don't deadlock.

    auto it = m_pending_parents.find(m_next_input_level);
    if (it != m_pending_parents.end()) {
        if (it->second.first != nullptr) {
            // There IS an old parent
            size_t parent_output_port = GetPortIndex(m_next_input_level, Direction::Out);
            LOG_DEBUG(get_logger()) << "JMultilevelSourceArrow: Evicting parent " << it->second.first->GetEventStamp() << " to port " << parent_output_port;
            outputs.at(output_count++) = {it->second.first, parent_output_port};
            it->second.first = nullptr;
        }
    }
}

void JMultilevelSourceArrow::fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) {

    if (!m_finish_in_progress) {

        LOG_DEBUG(m_logger) << "Executing arrow " << get_name() << LOG_END;
        auto result = m_source->DoNext(input->shared_from_this());
        m_next_input_level = m_source->GetNextInputLevel();
        m_next_input_port = GetPortIndex(m_next_input_level, Direction::In);

        LOG_DEBUG(get_logger()) << "JMultilevelSourceArrow: Returned from DoNext(" << toString(input->GetLevel()) << "). Next input level is " << toString(m_next_input_level);

        if (result == JEventSource::Result::Success) {
            // We have a newly filled event we have to do something with

            if (input->GetLevel() == m_child_event_level) {
                // We acquired a child! Attach it to its parents and push it into the big wide world

                for (auto [level, parent_pair] : m_pending_parents) {
                    // Note that this only attaches parents that we already, so if the parents arrive in the wrong order they
                    // will just be missing. If this is expected behavior, you'll need to set your downstream parent inputs to be optional.
                    if (parent_pair.first != nullptr) {
                        LOG_DEBUG(get_logger()) << "JMultilevelSourceArrow: Attaching parent: " << parent_pair.first->GetEventStamp() << " to event " << input->GetEventStamp();
                        input->SetParent(parent_pair.first);
                    }
                }
                outputs.at(output_count++) = {input, GetPortIndex(m_child_event_level, Direction::Out)};

                if (m_next_input_level != m_child_event_level) {
                    // We have to evict the parent AFTER the successful child because the child still needs the references to that parent
                    EvictNextParent(outputs, output_count);
                }
                status = JArrow::FireResult::KeepGoing;
                return;
            }
            else {
                // We acquired a parent event! It's predecessor SHOULD have already been evicted during the previous call to fire()

                if (m_next_input_level != m_child_event_level) {
                    // Evict the _subsequent_ parent
                    EvictNextParent(outputs, output_count);
                }

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
                    status = JArrow::FireResult::KeepGoing;
                    return;
                }
            }
        }
        else if (result == JEventSource::Result::FailureTryAgain) {
            if (m_next_input_level != m_child_event_level) {
                EvictNextParent(outputs, output_count);
            }
            // Return this event to the pool with no further action
            outputs.at(output_count++) = {input, GetPortIndex(input->GetLevel(), Direction::In)};
            status = JArrow::FireResult::ComeBackLater;
            return;
        }
        else if (result == JEventSource::Result::FailureLevelChange) {
            if (m_next_input_level != m_child_event_level) {
                EvictNextParent(outputs, output_count);
            }
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
