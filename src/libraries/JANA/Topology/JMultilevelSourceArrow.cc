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

void JMultilevelSourceArrow::fire(JEvent* input, OutputData& outputs, size_t& output_count, JArrow::FireResult& status) {

    if (m_deferred_finish) {
        // We need to dump all of our parent events when the event source finishes.
        // If there are more than two of them, we need to paginate them across multiple
        // calls to fire().
        // As long as more remain, we return FireResult::KeepGoing.
        // Once they are all gone, we return FireResult::Finished.
        return;
    }

    auto result = DoNext(*input);
    if (result == JEventSource::Result::Success) {
        // We have a newly filled event we have to do something with

        if (input->GetLevel() == m_child_event_level) {
            // We acquired a child! Attach it to its parents and push it into the big wide world
       
            for (auto [level, parent_pair] : m_pending_parents) {
                input->SetParent(parent_pair.first);
                // Note that this only attaches parents that we already, so if the parents arrive in the wrong order they
                // will just be missing. If this is expected behavior, you'll need to set your downstream parent inputs to be optional.
            }
            outputs.at(0) = {input, GetPortIndex(m_child_event_level, Direction::Out)};
            output_count = 1;
            status = JArrow::FireResult::KeepGoing;
            return;
        }
        else {
            // We acquired a parent event! It's predecessor SHOULD have already been evicted as soon as the source returned FailureLevelChange

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
            }
        }
    }
    else if (result == JEventSource::Result::FailureTryAgain) {
        // Return this event to the pool with no further action
        outputs.at(0) = {input, GetPortIndex(input->GetLevel(), Direction::In)};
        output_count = 1;
        status = JArrow::FireResult::ComeBackLater;
        return;
    }
    else if (result == JEventSource::Result::FailureLevelChange) {
        // Request input from a different port next time
        m_next_input_port = GetPortIndex(m_next_input_level, Direction::In);

        // Return this event to the pool
        outputs.at(0) = {input, GetPortIndex(input->GetLevel(), Direction::In)};
        output_count = 1;

        // This is a little bit tricky: Ideally we would be able to constrain max_inflight_events:$PARENT_LEVEL to be 1, 
        // which would essentially behave like (nicer) barrier events. However, if there is only 1 event
        // inflight at $PARENT_LEVEL, we have to evict immediately so that we don't deadlock.

        auto it = m_pending_parents.find(input->GetLevel());
        if (it != m_pending_parents.end()) {
            if (it->second.first != nullptr) {
                // There IS an old parent
                outputs.at(1) = {it->second.first, GetPortIndex(input->GetLevel(), Direction::Out)};
                it->second.first = nullptr;
                output_count = 2;
            }
        }

        status = JArrow::FireResult::KeepGoing;
        return;
    }
    else if (result == JEventSource::Result::FailureFinished) {
        // Return input to pool, but emit _all_ pending parents. This may require deferring finish
        // if more than two parents are present
        //    Pop two parents and send onward
        //    Set m_deferred_finish
        //    return FireResult::KeepGoing
        // else 
        //    Pop all remaining parents and send onward
        //    return FireResult::FailureFinished
        status = JArrow::FireResult::Finished;
    }
}
