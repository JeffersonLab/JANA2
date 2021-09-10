
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "SimpleClusterFactory.h"
#include "Hit.h"

#include <JANA/JEvent.h>

void SimpleClusterFactory::Init() {
    // auto app = GetApplication();
    
    /// Acquire any parameters
    // app->GetParameter("parameter_name", m_destination);
    
    /// Acquire any services
    // m_service = app->GetService<ServiceT>();
    
    /// Set any factory flags
    // SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER);
}

void SimpleClusterFactory::ChangeRun(const std::shared_ptr<const JEvent> &event) {
    /// This is automatically run before Process, when a new run number is seen
    /// Usually we update our calibration constants by asking a JService
    /// to give us the latest data for this run number
    
    // auto run_nr = event->GetRunNumber();
    // m_calibration = m_service->GetCalibrationsForRun(run_nr);
}

void SimpleClusterFactory::Process(const std::shared_ptr<const JEvent> &event) {

    /// JFactories are local to a thread, so we are free to access and modify
    /// member variables here. However, be aware that events are _scattered_ to
    /// different JFactory instances, not _broadcast_: this means that JFactory 
    /// instances only see _some_ of the events. 
    
    /// Acquire inputs (This may recursively call other JFactories)
    auto hits = event->Get<Hit>();

    /// Do some computation
    auto cluster = new Cluster(0,0,0,0,0);
    for (auto hit : hits) {
        cluster->x_center += hit->x;
        cluster->y_center += hit->y;
        cluster->E_tot += hit->E;
        if (cluster->t_begin > hit->t) cluster->t_begin = hit->t;
        if (cluster->t_end < hit->t) cluster->t_end = hit->t;
    }
    cluster->x_center /= hits.size();
    cluster->y_center /= hits.size();

    /// Publish outputs
    std::vector<Cluster*> results;
    results.push_back(cluster);
    Set(results);
}
