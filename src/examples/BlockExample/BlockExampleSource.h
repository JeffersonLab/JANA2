
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
	JLogger m_logger {JLogger::Level::DEBUG};

	virtual void Initialize() {
		LOG_INFO(m_logger) <<  "Initializing JBlockedEventSource" << LOG_END;
	}

	virtual Status NextBlock(MyBlock& block) {
		LOG_DEBUG(m_logger) <<  "BlockSource: Emitting block " << m_block_number << LOG_END;

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

		LOG_DEBUG(m_logger) <<  "BlockDisentangler: Disentangling block " << block.block_number << LOG_END;
		std::vector<std::shared_ptr<JEvent>> events;
		bool result = pool.get_many(events, block.data.size());

		if (result == false) {

			// TODO: Change this method signature so that we are automatically given the correct number of events
			//       instead of having to interact with the JEventPool ourselves.

			throw JException("Unable to obtain events from pool! Set jana:limit_events_in_flight=false");
		}

		size_t i = 0;
		for (auto datum : block.data) {
			LOG_DEBUG(m_logger) << "BlockDisentangler: extracted event containing " << datum << LOG_END;
			events[i++]->Insert(new MyObject(datum));
		}
		return events;
	}

};


#endif //JANA2_BLOCKEXAMPLESOURCE_H
