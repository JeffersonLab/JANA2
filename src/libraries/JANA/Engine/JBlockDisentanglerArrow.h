
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JBLOCKDISENTANGLERARROW_H
#define JANA2_JBLOCKDISENTANGLERARROW_H

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>
#include <JANA/JBlockedEventSource.h>

template <typename T>
class JBlockDisentanglerArrow : public JArrow {
	JBlockedEventSource<T>* m_source;  // non-owning
	JMailbox<T*>* m_block_queue; // owning
	JMailbox<std::shared_ptr<JEvent>>* m_event_queue; // non-owning
	std::shared_ptr<JEventPool> m_pool;

	size_t m_max_events_per_block = 40;

public:
	JBlockDisentanglerArrow(std::string name,
							JBlockedEventSource<T>* source,
							JMailbox<T*>* block_queue,
							JMailbox<std::shared_ptr<JEvent>>* event_queue,
							std::shared_ptr<JEventPool> pool
							)
							: JArrow(std::move(name), true, NodeType::Stage, 1)
							, m_source(source)
							, m_block_queue(block_queue)
							, m_event_queue(event_queue)
							, m_pool(std::move(pool))
							{}

	~JBlockDisentanglerArrow() {
		delete m_block_queue;
	}

	void set_max_events_per_block(size_t max_events_per_block) {
		m_max_events_per_block = max_events_per_block;
	}


	void execute(JArrowMetrics& result, size_t location_id) final {

                if (get_state() != State::Running) {
                    result.update_finished();
                    throw JException("I wonder if we actually get here. Do we want to?");
                    return;
                }
		JArrowMetrics::Status status;
		JArrowMetrics::duration_t latency;
		JArrowMetrics::duration_t overhead; // TODO: Populate these
		size_t message_count = 0;

		int requested_events = this->get_chunksize() * m_max_events_per_block; // chunksize is measured in blocks
		int reserved_events = m_event_queue->reserve(requested_events, location_id);
		int reserved_blocks = reserved_events / m_max_events_per_block; // truncate

		std::vector<T*> block_buffer; // TODO: Get rid of allocations
		std::vector<std::shared_ptr<JEvent>> event_buffer;

		auto input_queue_status = m_block_queue->pop(block_buffer, reserved_blocks, location_id);
		for (auto block : block_buffer) {
			auto events = m_source->DisentangleBlock(*block, *m_pool);
			event_buffer.insert(event_buffer.end(), events.begin(), events.end());
		}

		auto output_queue_status = m_event_queue->push(event_buffer, reserved_events, location_id);

		if (reserved_events == 0 || input_queue_status == JMailbox<T*>::Status::Congested || input_queue_status == JMailbox<T*>::Status::Full) {
			status = JArrowMetrics::Status::ComeBackLater;
		}
		// else if (input_queue_status == JMailbox<T*>::Status::Finished) {
		// 	status = JArrowMetrics::Status::Finished;
		// }
		else {
			status = JArrowMetrics::Status::KeepGoing;
		}
		result.update(status, reserved_blocks, 1, latency, overhead);
	}

};


#endif //JANA2_JBLOCKDISENTANGLERARROW_H
