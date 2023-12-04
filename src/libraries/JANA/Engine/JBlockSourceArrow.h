
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JBLOCKSOURCEARROW_H
#define JANA2_JBLOCKSOURCEARROW_H

#include <JANA/Engine/JPipelineArrow.h>
#include <JANA/Engine/JMailbox.h>
#include <JANA/JBlockedEventSource.h>

template <typename T>
class JBlockSourceArrow : public JPipelineArrow<JBlockSourceArrow<T>, T> {
	JBlockedEventSource<T>* m_source;  // non-owning

public:
	JBlockSourceArrow(std::string name, JBlockedEventSource<T>* source, JPool<T>* pool, JMailbox<T*>* block_queue)
		: JPipelineArrow<JBlockSourceArrow<T>,T>(name, false, JArrow::NodeType::Source, nullptr, block_queue, pool)
		, m_source(source)
	{}

	void initialize() final {
		m_source->Initialize();
	}

    void process(T* block, bool& success, JArrowMetrics::Status& status) {
		using Status = typename JBlockedEventSource<T>::Status;
		Status lambda_result = m_source->NextBlock(*block);
        if (lambda_result == Status::Success) {
            success = true;
            status = JArrowMetrics::Status::KeepGoing;
        }
        else if (lambda_result == Status::FailTryAgain) {
            success = false;
            status = JArrowMetrics::Status::ComeBackLater;
        }
        else if (lambda_result == Status::FailFinished) {
            success = false;
            status = JArrowMetrics::Status::Finished;
        }
	}
};


#endif //JANA2_JBLOCKSOURCEARROW_H
