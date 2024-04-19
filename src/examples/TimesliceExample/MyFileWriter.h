
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>
#include <podio/ROOTFrameWriter.h>
#include "CollectionTabulators.h"
#include <JANA/JEventProcessor.h>

#include <set>



struct MyFileWriter : public JEventProcessor {

    PodioInput<ExampleHit> m_ts_hits_in {this, "hits", JEventLevel::Timeslice};
    PodioInput<ExampleCluster> m_ts_protoclusters_in {this, "ts_protoclusters", JEventLevel::Timeslice};
    PodioInput<ExampleCluster> m_evt_protoclusters_in {this, "evt_protoclusters", JEventLevel::Event};
    PodioInput<ExampleCluster> m_evt_clusters_in {this, "clusters", JEventLevel::Event};

    Input<podio::Frame> m_evt_frame_in {this, "", JEventLevel::Event};
    Input<podio::Frame> m_ts_frame_in {this, "", JEventLevel::Timeslice};

    std::unique_ptr<podio::ROOTFrameWriter> m_writer = nullptr;
    std::set<int> m_seen_event_nrs;
    int m_expected_timeslice_count;

    std::mutex m_mutex;
    
    MyFileWriter() {
        SetTypeName(NAME_OF_THIS);
    }

    void Init() {
        m_writer = std::make_unique<podio::ROOTFrameWriter>("output.root");
        m_expected_timeslice_count = GetApplication()->GetParameterValue<int>("jana:nevents");
    }

    void Process(const std::shared_ptr<const JEvent>& event) {

        auto ts_nr = event->GetParent(JEventLevel::Timeslice).GetEventNumber();
        m_seen_event_nrs.insert(event->GetEventNumber());

        std::lock_guard<std::mutex> guard(m_mutex);
        m_writer->writeFrame(*(m_evt_frame_in().at(0)), "events");
        if (event->GetEventIndex() == 0) {
            m_writer->writeFrame(*(m_ts_frame_in().at(0)), "timeslices");
        }

        LOG_DEBUG(GetLogger()) 
            << "MyEventProcessor: Event " << event->GetEventNumber() << " from Timeslice " << ts_nr
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

    void Finish() {
        m_writer->finish();
        for (int tsnr=0; tsnr<m_expected_timeslice_count; ++tsnr) {
            for (int chnr=0; chnr < 3; ++chnr) {
                int evtnr = 100*tsnr + chnr;

                if (!m_seen_event_nrs.contains(evtnr)) {
                    LOG << "MyEventProcessor: Missing event #" << evtnr << LOG_END;
                }
            }
        }
    }
};


