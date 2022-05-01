
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_BLOCKEXAMPLESOURCE_H
#define JANA2_BLOCKEXAMPLESOURCE_H

#include <JANA/JBlockedEventSource.h>
#include <JANA/JLogger.h>

#include "MyBlock.h"
#include "MyObject.h"

class BlockExampleSource : public JBlockedEventSource<MyBlock> {

	int m_block_number = 1;
	JLogger m_logger;

	virtual void Initialize() {
		LOG_INFO(m_logger) <<  "Initializing JBlockedEventSource" << LOG_END;
	}

	virtual Status NextBlock(MyBlock& block) {
		LOG_DEBUG(m_logger) <<  "JBlockedEventSource::NextBlock" << LOG_END;

		if (m_block_number >= 10) {
			return Status::FailFinished;
		}

		block.block_number = m_block_number++;
		block.data.push_back(block.block_number*10 + 1);
		block.data.push_back(block.block_number*10 + 2);
		block.data.push_back(block.block_number*10 + 3);
		return Status::Success;
	}

	virtual std::vector<std::shared_ptr<JEvent>> DisentangleBlock(MyBlock& block, JEventPool& pool) {

		LOG_DEBUG(m_logger) <<  "JBlockedEventSource::DisentangleBlock" << LOG_END;
		std::vector<std::shared_ptr<JEvent>> events;
		for (auto datum : block.data) {
			auto event = pool.get(0);  // TODO: Make location be transparent to end user
			event->Insert(new MyObject(datum));
			events.push_back(event);
		}
		return events;
	}

};


#endif //JANA2_BLOCKEXAMPLESOURCE_H
