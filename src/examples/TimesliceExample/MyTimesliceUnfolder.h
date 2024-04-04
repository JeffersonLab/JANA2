// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <DatamodelGlue.h>

#include <JANA/JEventUnfolder.h>
#include "CollectionTabulators.h"

struct MyTimesliceUnfolder : public JEventUnfolder {

    PodioInput<ExampleCluster> m_timeslice_clusters_in {this, "ts_protoclusters", JEventLevel::Timeslice};
    PodioOutput<ExampleCluster> m_event_clusters_out {this, "evt_protoclusters"};

    size_t next_time_bucket = 0;

    MyTimesliceUnfolder() {
        SetTypeName(NAME_OF_THIS);
        SetParentLevel(JEventLevel::Timeslice);
        SetChildLevel(JEventLevel::Event);
    }


    Result Unfold(const JEvent& parent, JEvent& child, int child_idx) override {

        auto timeslice_nr = parent.GetEventNumber();
        size_t event_nr = 100*timeslice_nr + child_idx;
        child.SetEventNumber(event_nr);

        // For now, a one-to-one relationship between timeslices and events

        auto event_clusters_out = std::make_unique<ExampleClusterCollection>();
        event_clusters_out->setSubsetCollection(true);
        event_clusters_out->push_back(m_timeslice_clusters_in()->at(child_idx));

        LOG_DEBUG(GetLogger()) << "MyTimesliceUnfolder: Timeslice " << parent.GetEventNumber() 
            <<  ", Event " << child.GetEventNumber()
            << "\nTimeslice clusters in:\n"
            << TabulateClusters(m_timeslice_clusters_in())
            << "\nEvent clusters out:\n"
            << TabulateClusters(event_clusters_out.get())
            << LOG_END;

        m_event_clusters_out() = std::move(event_clusters_out);

        return (child_idx == 2) ? Result::NextChildNextParent : Result::NextChildKeepParent;
    }
};



