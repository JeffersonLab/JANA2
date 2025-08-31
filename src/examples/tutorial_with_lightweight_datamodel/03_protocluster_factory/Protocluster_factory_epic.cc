
#include "Protocluster_factory_epic.h"
#include "Protocluster_algorithm.h"


Protocluster_factory_epic::Protocluster_factory_epic() {

    // Set the type name of this class, in order to have more informative error messages
    SetTypeName(NAME_OF_THIS);

    // Set the default prefix. This is a unique name for this factory that is used as a prefix for parameters.
    // In ePIC, this is usually overridden by the JOmniFactoryGenerator
    SetPrefix("protoclusterizer");

    // JOmniFactories support multiple outputs, and each of these outputs needs one or more databundle names.
    // Set these in the constructor, because is critical that they are set _before_ Init() and Configure() get called.
    // You can provide either a short name (which only has to be unique relative to the output type name),
    // or a globally unique name. If you provide a short name, the unique name will be "Typename:Shortname",
    // e.g. "CalorimeterCluster:protoclusters".
    //
    // GlueX uses short names, whereas ePIC uses unique names. Favor unique names when using Podio, for two reasons:
    // 1. The collection names in the podio file have to be fully unique, so this way the code matches the datafile.
    // 2. edm4hep/edm4eic provides datamodel classes that are meant to be reused in a wide variety of contexts, so you
    //      end up needing very descriptive names regardless. Contrast this with GlueX, where each detector has a unique
    //      class for each data product in the reconstruction chain, and the short name usually ends up being "".
    //
    // Short names and unique names are fully interoperable, so you can mix code from different experiments. For instance,
    // if you were testing an ePIC detector parasitically in Hall D, you'd want to use the GlueX DAQ with ePIC tracking.

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

void Protocluster_factory_epic::Configure() {
    // Configure() is called sequentially before any processing starts. It is safe to modify state
    // that is not a member variable of the factory itself.

    // By the time Configure() is called,
    // - Logger has been set
    // - Declared Parameter values have been fetched
    // - Declared Services have been fetched

    LOG_DEBUG(GetLogger()) << "Inside Configure()";

    // Unlike v1, you don't have to fetch any parameters or services in here, as this is done automatically now.
    // However, you may wish to fetch data from your JServices, if it isn't keyed off of the run number.
    // This is also where you should initialize your algorithm, if necessary.
}

void Protocluster_factory_epic::ChangeRun(int32_t run_number) {
    LOG_DEBUG(GetLogger()) << "Inside ChangeRun() with run_number=" << run_number;
    // This is where you should fetch any data from your JServices that IS keyed off of the run number
}

void Protocluster_factory_epic::Execute(int32_t run_number, uint64_t event_number) {
    LOG_DEBUG(GetLogger()) << "Inside Execute() with run_number=" << run_number 
                          << ", event_number=" << event_number;


    // The Input helpers will already have been filled by the time Execute() gets called. You can access
    // the data using the () operator. Parameter values may be accessed one of two ways:
    //   config().energy_threshold
    //   m_energy_threshold()

    m_clusters_out() = calculate_protoclusters(m_hits_in(), config().log_weight_energy);

    // While you are inside Execute(), you can populate your output databundles however you like. Once Execute()
    // returns, JANA2 will store and retrieve them automatically.

}


