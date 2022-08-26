
#include "Cluster_factory_Simple.h"
#include "Hit.h"

#include <JANA/JEvent.h>

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Please add the following lines to your InitPlugin or similar routine
// in order to register this factory with the system.
//
// #include "Cluster_factory_Simple.h"
//
//     app->Add( new JFactoryGeneratorT<Cluster_factory_Simple>() );
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 


//------------------------
// Constructor
//------------------------
Cluster_factory_Simple::Cluster_factory_Simple(){
    SetTag("Simple");
}

//------------------------
// Init
//------------------------
void Cluster_factory_Simple::Init() {
    auto app = GetApplication();
    
    /// Acquire any parameters
    // app->GetParameter("parameter_name", m_destination);
    
    /// Acquire any services
    // m_service = app->GetService<ServiceT>();
    
    /// Set any factory flags
    // SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER);
}

//------------------------
// ChangeRun
//------------------------
void Cluster_factory_Simple::ChangeRun(const std::shared_ptr<const JEvent> &event) {
    /// This is automatically run before Process, when a new run number is seen
    /// Usually we update our calibration constants by asking a JService
    /// to give us the latest data for this run number
    
    auto run_nr = event->GetRunNumber();
    // m_calibration = m_service->GetCalibrationsForRun(run_nr);
}

//------------------------
// Process
//------------------------
void Cluster_factory_Simple::Process(const std::shared_ptr<const JEvent> &event) {

    auto hits = event->Get<Hit>();

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

    std::vector<Cluster*> results;
    results.push_back(cluster);
    Set(results);
}
