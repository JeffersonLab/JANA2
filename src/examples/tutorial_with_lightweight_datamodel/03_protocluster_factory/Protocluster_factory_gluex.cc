
#include "Protocluster_factory_gluex.h"
#include "Protocluster_algorithm.h"

#include <JANA/JEvent.h>
#include <CalorimeterHit.h>


Protocluster_factory_gluex::Protocluster_factory_gluex() {
    // Remember the type name of this class, in order to have more informative error messages
    SetTypeName(NAME_OF_THIS);

    // Set the tag for this factory's only output. Tags only need to be unique up to the given type,
    // so they can be short. If you don't set the tag, it will default to the empty string.
    SetTag("proto");

    // Set any factory flags, e.g.
    //   SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER);
    //   SetNotOwnerFlag(true); // <-- New and improved style
}


void Protocluster_factory_gluex::Init() {
    // Init() is called sequentially before any processing starts. It is safe to modify state
    // that is not a member variable of the factory itself.

    // By the time Init() is called, the logger has been configured
    LOG_DEBUG(GetLogger()) << "Inside Protocluster_factory_gluex::Init()";

    // Acquire the JApplication, which gives access to parameters and services
    auto app = GetApplication();

    // Acquire any parameters
    app->SetDefaultParameter("protoclusterizer:log_weight_energy", m_log_weight_energy,
                             "Energy threshold in [units]");

    // Acquire any services
    // Not needed here, see 11_calibrations for an example
    // m_service = app->GetService<ServiceT>();
}


void Protocluster_factory_gluex::ChangeRun(const std::shared_ptr<const JEvent> & event) {
    // ChangeRun() is called in parallel during processing. It is NOT safe to modify any state
    // that is not a member variable of the factory itself.
    //
    // This is where you should update the values of any calibrations your factory is using
    // Not needed here, see 11_calibrations for an example

    LOG_DEBUG(GetLogger()) << "Inside Protocluster_factory_gluex::ChangeRun() with run_number=" << event->GetRunNumber();
}


void Protocluster_factory_gluex::Process(const std::shared_ptr<const JEvent> &event) {
    // Process() is called in parallel during processing. Any data cached as member variables on the factory
    // instance is safe to read and write. Static or global data is generally NOT safe to access. If you
    // absolutely must access static or global data, expose it using a JService and make that access thread-safe.

    LOG_DEBUG(GetLogger()) << "Inside Protocluster_factory_gluex::Process() with run_number=" << event->GetRunNumber()
                           << ", event_number=" << event->GetEventNumber();

    // Retrieve each input directly from the JEvent
    auto hits = event->Get<CalorimeterHit>("rechits");

    // Run the algorithm (Note that it is okay if you put all the algorithm's code directly inside Process())
    auto clusters = calculate_protoclusters(hits, m_log_weight_energy);

    // Store the output collection
    Set(clusters);
    // Note that some factories store the outputs by accessing the `mData` field directly:
    //   mData = std::move(clusters);
}


void Protocluster_factory_gluex::Finish() {
    // Finish() is called sequentially after all processing has stopped and JANA2 is shutting down.
    // You rarely need Finish(). In GlueX, Finish() is mainly used for filling histograms of factory-specific metadata.
    // Moving forward, factory-specific metadata should always be exposed as a separate output, and the histogram should be
    // written downstream, the same as all of the other histograms.

    LOG_DEBUG(GetLogger()) << "Inside Protocluster_factory_gluex::Finish()";
}

