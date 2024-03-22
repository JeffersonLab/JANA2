
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>

#include <JANA/JEventProcessor.h>

struct MyEventProcessor : public JEventProcessor {

    PodioInput<ExampleCluster> clusters_in {this, "clusters", JEventLevel::Event};

    std::mutex m_mutex;
    
    MyEventProcessor() {
        SetLevel(JEventLevel::Event);
        SetTypeName("MyEventProcessor");
    }

    void Process(const std::shared_ptr<const JEvent>& event) {

        std::lock_guard<std::mutex> guard(m_mutex);

    }
};


