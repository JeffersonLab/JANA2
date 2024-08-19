
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_GROUPEDEVENTPROCESSOR_H
#define JANA2_GROUPEDEVENTPROCESSOR_H


#include <JANA/JEventProcessor.h>
#include <JANA/JLogger.h>
#include <JANA/Services/JEventGroupTracker.h>
#include <JANA/Utils/JPerfUtils.h>

#include "TridasEvent.h"

/// GroupedEventProcessor demonstrates basic usage of JEventGroups

class GroupedEventProcessor : public JEventProcessor {

public:
    GroupedEventProcessor() {
        SetTypeName(NAME_OF_THIS);
        SetCallbackStyle(CallbackStyle::LegacyMode);
    }

    void Process(const std::shared_ptr<const JEvent>& event) override {

        // In parallel, perform a random amount of (slow) computation
        consume_cpu_ms(100, 1.0);

        auto tridas_event = event->GetSingle<TridasEvent>();
        tridas_event->should_keep = true;

        auto group = event->GetSingle<JEventGroup>();

        // Sequentially, process each event and report when a group finishes
        std::lock_guard<std::mutex> lock(m_mutex);

        LOG << "Processed group #" << group->GetGroupId() << ", event #" << event->GetEventNumber() << LOG_END;

        bool finishes_group = group->FinishEvent();
        if (finishes_group) {
            LOG << "Processed last element in group " << group->GetGroupId() << LOG_END;
        }
    }
};




#endif //JANA2_GROUPEDEVENTPROCESSOR_H
