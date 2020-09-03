//
// Created by Nathan Brei on 8/21/20.
//

#ifndef JANA2_JBLOCKEDEVENTSOURCEARROW_H
#define JANA2_JBLOCKEDEVENTSOURCEARROW_H

#include <JANA/Engine/JArrow.h>
#include <JANA/Engine/JMailbox.h>
#include <JANA/JBlockedEventSource.h>

template <typename T>
class JBlockSourceArrow : public JArrow {
	JBlockedEventSource<T>* m_source;  // non-owning
	JMailbox<T>* m_block_queue; // non-owning
	JLogger m_logger;

public:
	JBlockSourceArrow(std::string name, JBlockedEventSource<T>* source, JMailbox<T>* block_queue)
		: JArrow(name, false, NodeType::Source, 1)
		, m_source(source)
		, m_block_queue(block_queue)
	{}

	void initialize() final {
		LOG_DEBUG(m_logger) << "JBlockDisentanglerArrow '" << get_name() << "': " << "Initializing" << LOG_END;
		assert(_status == Status::Unopened);
		//_source->Initialize();
		_status = Status::Running;
	}
	void execute(JArrowMetrics& result, size_t location_id) final {

	}
};



template <typename T>
class JBlockDisentanglerArrow : public JArrow {
	JBlockedEventSource<T>* m_source;  // non-owning
	JMailbox<T>* m_block_queue; // owning
	JMailbox<std::shared_ptr<JEvent>>* m_event_queue; // non-owning
	std::shared_ptr<JEventPool> m_pool;
	JLogger m_logger;

public:
	JBlockDisentanglerArrow(std::string name,
							JBlockedEventSource<T>* source,
							JMailbox<T>* block_queue,
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

	void initialize() final {
		LOG_DEBUG(m_logger) << "JBlockDisentanglerArrow '" << get_name() << "': " << "Initializing" << LOG_END;
		assert(_status == Status::Unopened);
		//_source->InitializeDisentangler();
		_status = Status::Running;
	};

	void execute(JArrowMetrics& result, size_t location_id) final {

	}

};


#endif //JANA2_JBLOCKEDEVENTSOURCEARROW_H
