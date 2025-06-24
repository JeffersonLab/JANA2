// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JEventUnfolder.h>
#include <PodioDatamodel/EventInfoCollection.h>
#include "CollectionTabulators.h"

struct MyTimesliceSplitter : public JEventUnfolder {

    PodioInput<ExampleCluster> m_timeslice_clusters_in {this, {.name = "ts_protoclusters", 
                                                               .level = JEventLevel::Timeslice}};

    PodioOutput<ExampleCluster> m_event_clusters_out {this, "evt_protoclusters"};
    PodioOutput<EventInfo> m_event_info_out {this, "evt_info"};

    MyTimesliceSplitter() {
        SetTypeName(NAME_OF_THIS);
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::PhysicsEvent);
    }


    Result Unfold(const JEvent& parent, JEvent& child, int child_idx) override {

        auto timeslice_nr = parent.GetEventNumber();
        size_t event_nr = 100*timeslice_nr + child_idx;
        child.SetEventNumber(event_nr);

        // For now, a one-to-one relationship between timeslices and events

        auto event_clusters_out = std::make_unique<ExampleClusterCollection>();
        event_clusters_out->setSubsetCollection(true);
        event_clusters_out->push_back(m_timeslice_clusters_in()->at(child_idx));

        auto event_info_out = std::make_unique<EventInfoCollection>();
        event_info_out->push_back(MutableEventInfo(event_nr, timeslice_nr, 0));

        LOG_DEBUG(GetLogger()) << "MyTimesliceSplitter: Timeslice " << parent.GetEventNumber() 
            <<  ", Event " << child.GetEventNumber()
            << "\nTimeslice clusters in:\n"
            << TabulateClusters(m_timeslice_clusters_in())
            << "\nEvent clusters out:\n"
            << TabulateClusters(event_clusters_out.get())
            << LOG_END;

        m_event_clusters_out() = std::move(event_clusters_out);
        m_event_info_out() = std::move(event_info_out);

        return (child_idx == 2) ? Result::NextChildNextParent : Result::NextChildKeepParent;
    }
};



