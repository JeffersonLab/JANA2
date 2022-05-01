
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JBLOCKEDEVENTSOURCE_H
#define JANA2_JBLOCKEDEVENTSOURCE_H

#include <JANA/JEvent.h>
#include <JANA/Utils/JEventPool.h>

template <typename BlockType>
class JBlockedEventSource {
public:

	enum class Status { Success, FailTryAgain, FailFinished };

	virtual void Initialize() {}

	virtual Status NextBlock(BlockType& block) = 0;

	virtual std::vector<std::shared_ptr<JEvent>> DisentangleBlock(BlockType& block, JEventPool& pool) = 0;
};


#endif //JANA2_JBLOCKEDEVENTSOURCE_H
