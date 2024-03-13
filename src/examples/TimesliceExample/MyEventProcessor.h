
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include "MyDataModel.h"
#include <JANA/JEventProcessor.h>

struct ExampleEventProcessor : public JEventProcessor {

    std::mutex m_mutex;
    
    ExampleTimesliceProcessor() {
        SetEventLevel(JEvent::Level::Event);
    }

    void Process(const std::shared_ptr<const JEvent>& event) {

        std::lock_guard<std::mutex> guard(m_mutex);

        auto outputs = event->Get<MyOutput>();
        // assert(outputs.size() == 4);
        // assert(outputs[0]->z == 25.6f);
        // assert(outputs[1]->z == 26.5f);
        // assert(outputs[2]->z == 27.4f);
        // assert(outputs[3]->z == 28.3f);
        LOG << " Contents of event " << event->GetEventNumber() << LOG_END;
        for (auto output : outputs) {
            LOG << " " << output->evt << ":" << output->sub << " " << output->z << LOG_END;
        }
        LOG << " DONE with contents of event " << event->GetEventNumber() << LOG_END;
    }
};


