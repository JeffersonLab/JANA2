
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>
#include <JANA/JEventProcessor.h>
#include "CollectionTabulators.h"

struct MyEventProcessor : public JEventProcessor {

    PodioInput<ExampleHit> m_ts_hits_in {this, "hits", JEventLevel::Timeslice};
    PodioInput<ExampleCluster> m_ts_protoclusters_in {this, "ts_protoclusters", JEventLevel::Timeslice};
    PodioInput<ExampleCluster> m_evt_protoclusters_in {this, "evt_protoclusters", JEventLevel::Event};
    PodioInput<ExampleCluster> m_evt_clusters_in {this, "clusters", JEventLevel::Event};

    std::mutex m_mutex;
    
    MyEventProcessor() {
        SetLevel(JEventLevel::Event);
        SetTypeName("MyEventProcessor");
    }

    void Process(const std::shared_ptr<const JEvent>& event) {

        std::lock_guard<std::mutex> guard(m_mutex);
        auto ts_nr = event->GetParent(JEventLevel::Timeslice).GetEventNumber();

        LOG << "MyEventProcessor: Event " << event->GetEventNumber() << " from Timeslice " << ts_nr
            << "\nTimeslice-level hits\n"
            << TabulateHits(m_ts_hits_in())
            << "\nTimeslice-level protoclusters\n"
            << TabulateClusters(m_ts_protoclusters_in())
            << "\nEvent-level protoclusters\n"
            << TabulateClusters(m_evt_protoclusters_in())
            << "\nEvent-level clusters\n"
            << TabulateClusters(m_evt_clusters_in())
            << LOG_END;
    }
};


