//
// Created by Nathan Brei on 8/21/20.
//

#ifndef JANA2_JBLOCKEDEVENTSOURCEARROW_H
#define JANA2_JBLOCKEDEVENTSOURCEARROW_H

#include <JANA/Engine/JArrow.h>
#include <JANA/JBlockedEventSource.h>

class JBlockSourceArrow : public JArrow {
	JBlockedEventSource* m_source;  // non-owning
	JLogger m_logger;

public:
	JEventSourceArrow(std::string name, JEventSource* source, EventQueue* output_queue, std::shared_ptr<JEventPool> pool);
	void initialize() final;
	void execute(JArrowMetrics& result, size_t location_id) final;

};

class JBlockDisentanglerArrow : public JArrow {
	JBlockedEventSource* m_source;  // non-owning
	std::shared_ptr<JEventPool> m_pool;
	JLogger m_logger;

public:
	JEventSourceArrow(std::string name, JEventSource* source, EventQueue* output_queue, std::shared_ptr<JEventPool> pool);
	void initialize() final;
	void execute(JArrowMetrics& result, size_t location_id) final;

};


#endif //JANA2_JBLOCKEDEVENTSOURCEARROW_H
