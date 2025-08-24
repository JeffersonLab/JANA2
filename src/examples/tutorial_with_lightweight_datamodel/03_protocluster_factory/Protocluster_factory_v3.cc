
#include "Protocluster_factory_v3.h"
#include "JANA/Utils/JTypeInfo.h"
#include "Protocluster_algorithm.h"


Protocluster_factory_v3::Protocluster_factory_v3() {


    // Set the type name of this class, in order to have more informative error messages
    SetTypeName(NAME_OF_THIS);

    // Set the default prefix. This is a unique name for this factory that is used as a prefix for parameters.
    // In ePIC, this is usually overridden by the JOmniFactoryGenerator
    SetPrefix("protoclusterizer");

    // Set the default output name(s). If you don't, they will default to "". Note that ePIC always sets their
    // output names in the JOmniFactoryGenerator instead.
    m_clusters_out.SetShortName("proto");

    // Set the default input name(s). If you don't, they will default to "". Note that ePIC always sets their input
    // names in the JOmniFactoryGenerator instead.
    m_hits_in.SetDatabundleName("raw"); // Can be either short name or unique name

    // If you declare an input as optional, JANA2 won't throw an exception if it can't find it. When you try to access
    // the missing input, it will present as an empty vector.
    //   m_hits_in.SetOptional(true);

    // Set factory flags here, as needed.
    //   m_clusters_out.SetNotOwnerFlag(true);
    // Note that when there are multiple outputs, the NOT_OBJECT_OWNER flag should be set on the Output, not on the factory itself.
    // Setting NOT_OBJECT_OWNER on a factory really means setting NOT_OBJECT_OWNER on the factory's _first_ output.

}

void Protocluster_factory_v3::Init() {
    // Init() is called sequentially before any processing starts. It is safe to modify state
    // that is not a member variable of the factory itself.

    // By the time Init() is called,
    // - Logger has been configured
    // - Declared Parameter values have been fetched
    // - Declared Services have been fetched

    LOG_DEBUG(GetLogger()) << "Inside Init()";

    // Unlike v1, you don't have to fetch any parameters or services in here, as this is done automatically now.
    // However, you may wish to fetch data from your JServices, if it isn't keyed off of the run number.
    // This is also where you should initialize your algorithm, if necessary.
}

void Protocluster_factory_v3::ChangeRun(const JEvent& event) {
    LOG_DEBUG(GetLogger()) << "Inside ChangeRun() with run_number=" << event.GetRunNumber();

    // This is where you should fetch any data from your JServices that IS keyed off of the run number
}

void Protocluster_factory_v3::Process(const JEvent& event) {
    LOG_DEBUG(GetLogger()) << "Inside Execute() with run_number=" << event.GetRunNumber()
                          << ", event_number=" << event.GetEventNumber();

    // The Input helpers will already have been filled by the time Execute() gets called. You can access
    // the data using the () operator. Parameter values may also be accessed using the () operator.

    m_clusters_out() = calculate_protoclusters(m_hits_in(), m_log_weight_energy());

    // While you are inside Execute(), you can populate your output databundles however you like. Once Execute()
    // returns, JANA2 will store and retrieve them automatically.

}

void Protocluster_factory_v3::Finish() {
    LOG_DEBUG(GetLogger()) << "Inside Finish()";
}


