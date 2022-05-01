
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "BlockExampleProcessor.h"
#include "MyObject.h"
#include <JANA/JLogger.h>

BlockExampleProcessor::BlockExampleProcessor() {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}

void BlockExampleProcessor::Init() {
    LOG << "BlockExampleProcessor::Init" << LOG_END;
    // Open TFiles, set up TTree branches, etc
}

void BlockExampleProcessor::Process(const std::shared_ptr<const JEvent> &event) {
    LOG << "BlockExampleProcessor::Process, Event #" << event->GetEventNumber() << LOG_END;
    
    auto objs = event->Get<MyObject>();
    std::lock_guard<std::mutex>lock(m_mutex);

	for (const MyObject* obj : objs) {
        LOG << obj->datum << LOG_END;
    }
}

void BlockExampleProcessor::Finish() {
    // Close any resources
    LOG << "BlockExampleProcessor::Finish" << LOG_END;
}

