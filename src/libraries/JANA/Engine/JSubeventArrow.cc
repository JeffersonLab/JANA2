
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JSubeventArrow.h"





JSubeventArrow::JSubeventArrow(std::string name, JSubeventProcessor* processor,
                               JMailbox<JSubevent>* inbox, JMailbox<JSubevent>* outbox)
    : JArrow(name, true, NodeType::Stage), m_processor(processor), m_inbox(inbox), m_outbox(outbox) {


}

void JSubeventArrow::execute(JArrowMetrics& results, size_t location_id) {
    // auto in_result = _inbox.pop(in_buffer, get_chunksize());
    // for (JObject* item : in_buffer) {
    //    _processor.process(item);
    // }
    // auto out_result = _outbox.push(out_buffer);
    // measure everything
}

JSplitArrow::JSplitArrow(std::string name, JSubeventProcessor* processor,
                         JMailbox<Event>* inbox, JMailbox<JSubevent>* outbox)
    : JArrow(name, true, NodeType::Stage), m_processor(processor), m_inbox(inbox), m_outbox(outbox) {

}

void JSplitArrow::execute(JArrowMetrics& results, size_t location_id) {
    // input_queue.pop(in_buffer, get_chunksize());
    // _processor.split(event, in_buffer);
    // size_t count = in_buffer.size();
    // for (size_t i=0; i<count; ++i) {
    //     out_buffer.push_back(JSubevent(event, buffer[i], i, count));
    // }
    // _outbox.push(out_buffer);
    // measure everything
}

JMergeArrow::JMergeArrow(std::string name, JSubeventProcessor* processor, JMailbox<JSubevent>* inbox, JMailbox<Event>* outbox)
    : JArrow(name, true, NodeType::Stage), m_processor(processor), m_inbox(inbox), m_outbox(outbox) {

}

void JMergeArrow::execute(JArrowMetrics& results, size_t location_id) {
    // This is the complicated one
    // For now, don't worry about allocations, or about queue overflow
    // Maintain a map of {parent, (count, [subevents])}
    // on each add, if subevents.size() = count, then
    //
    // Alternative: If item is not part of current event, then push back onto queue
}
