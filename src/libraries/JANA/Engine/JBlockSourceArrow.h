
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JBLOCKSOURCEARROW_H
#define JANA2_JBLOCKSOURCEARROW_H

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>
#include <JANA/JBlockedEventSource.h>

template <typename T>
class JBlockSourceArrow : public JArrow {
	JBlockedEventSource<T>* m_source;  // non-owning
	JMailbox<T>* m_block_queue; // non-owning
	JLogger m_logger;

	T* m_next_block = nullptr;

public:
	JBlockSourceArrow(std::string name, JBlockedEventSource<T>* source, JMailbox<T>* block_queue)
		: JArrow(name, false, NodeType::Source, 1)
		, m_source(source)
		, m_block_queue(block_queue)
	{}

	void initialize() final {
		LOG_DEBUG(m_logger) << "JBlockDisentanglerArrow '" << get_name() << "': " << "Initializing" << LOG_END;
		assert(_status == Status::Unopened);
		m_source->Initialize();
		_status = Status::Running;
	}

	void execute(JArrowMetrics& result, size_t location_id) final {

		if (!this->is_active()) {
			result.update_finished();
			return;
		}
		JArrowMetrics::Status status;
		JArrowMetrics::duration_t latency;
		JArrowMetrics::duration_t overhead; // TODO: Populate these
		size_t message_count = 0;

		int requested_count = this->get_chunksize();
		std::vector<T> chunk_buffer; // TODO: Get rid of allocations
		int reserved_count = this->m_outbox->reserve(requested_count, location_id);

		using Status = typename JBlockedEventSource<T>::Status;
		if (reserved_count != 0) {

			Status lambda_result = Status::Success;
			for (int i=0; i<reserved_count && lambda_result == Status::Success; ++i) {
				if (m_next_block == nullptr) {
					m_next_block = new T;
				}
				auto pair = m_source->NextBlock(m_next_block);
				lambda_result = pair.first;
				if (lambda_result == Status::Success) {
					chunk_buffer.push_back(m_next_block);
					m_next_block = nullptr;
				}
			}

			// We have to return our reservation regardless of whether our pop succeeded
			this->m_outbox->push(chunk_buffer, reserved_count, location_id);

			if (lambda_result == Status::Success) {
				status = JArrowMetrics::Status::KeepGoing;
			}
			else if (lambda_result == Status::FailTryAgain) {
				status = JArrowMetrics::Status::ComeBackLater;
				// TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
			}
			else if (lambda_result == Status::FailFinished) {
				status = JArrowMetrics::Status::Finished;
			}
		}
		else { // reserved_count = 0  => downstream is full
			status = JArrowMetrics::Status::ComeBackLater;
			// TODO: Consider reporting queueempty/queuefull/sourcebusy here instead
		}
		result.update(status, message_count, 1, latency, overhead);
	}
};


#endif //JANA2_JBLOCKSOURCEARROW_H
