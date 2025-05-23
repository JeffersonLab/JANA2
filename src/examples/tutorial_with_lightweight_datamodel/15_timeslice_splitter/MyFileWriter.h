
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JEventProcessor.h>
#include "CollectionTabulators.h"

#include <podio/podioVersion.h>
#if podio_VERSION_MAJOR == 0 && podio_VERSION_MINOR < 99
#include <podio/ROOTFrameWriter.h>
namespace podio {
using ROOTWriter = podio::ROOTFrameWriter;
}
#else
#include <podio/ROOTWriter.h>
#endif



struct MyFileWriter : public JEventProcessor {

    // Trigger the creation of clusters
    PodioInput<ExampleCluster> m_evt_clusters_in {this, {.name="clusters"}};

    // Retrieve the PODIO frame so we can write it directly
    Input<podio::Frame> m_evt_frame_in {this, {.name = "", 
                                               .level = JEventLevel::PhysicsEvent}};

    Input<podio::Frame> m_ts_frame_in {this, {.name = "", 
                                              .level = JEventLevel::Timeslice,
                                              .is_optional = true }};

    std::unique_ptr<podio::ROOTWriter> m_writer = nullptr;
    std::mutex m_mutex;
    
    MyFileWriter() {
        SetTypeName(NAME_OF_THIS);
        SetCallbackStyle(CallbackStyle::ExpertMode);
    }

    void Init() {
        m_writer = std::make_unique<podio::ROOTWriter>("output.root");
    }

    void Process(const JEvent& event) {

        std::lock_guard<std::mutex> guard(m_mutex);
        if (event.HasParent(JEventLevel::Timeslice)) {

            auto& ts = event.GetParent(JEventLevel::Timeslice);
            auto ts_nr = ts.GetEventNumber();

            if (event.GetEventIndex() == 0) {
                m_writer->writeFrame(*(m_ts_frame_in().at(0)), "timeslices");
            }

            LOG_DEBUG(GetLogger()) 
                << "Event " << event.GetEventNumber() << " from Timeslice " << ts_nr
                << "\nClusters\n"
                << TabulateClusters(m_evt_clusters_in())
                << LOG_END;
        }
        else {

            LOG_DEBUG(GetLogger()) 
                << "Event " << event.GetEventNumber()
                << "\nClusters\n"
                << TabulateClusters(m_evt_clusters_in())
                << LOG_END;
        }

        m_writer->writeFrame(*(m_evt_frame_in().at(0)), "events");

    }

    void Finish() {
        m_writer->finish();
    }
};


