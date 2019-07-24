//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

#include <JANA/JTopologyBuilder.h>
#include <JANA/JProcessingTopology.h>
#include <JANA/JEventProcessorArrow.h>
#include <JANA/JEventSourceArrow.h>
#include <JANA/JTopologyBuilder.h>

#include <unordered_set>
#include "JTopologyBuilder.h"


void JTopologyBuilder::add(std::string event_source_name) {
    _evt_src_names.push_back(event_source_name);
}

void JTopologyBuilder::add(JEventSourceGenerator* source_generator) {
    _evt_src_gens_front.push_back(source_generator);
}

void JTopologyBuilder::add(JFactoryGenerator* factory_generator) {
    _fac_gens_front.push_back(factory_generator);
}

void JTopologyBuilder::add(JEventProcessor* processor) {
    _evt_procs_front.push_back(processor);
}

void JTopologyBuilder::add(JEventSource* source) {
    _evt_srces_front.push_back(source);
}


void JTopologyBuilder::increase_priority() {

    // Append everything low-priority to back of high-priority lists,
    // clear the low-priority lists, and then swap the two

    for (auto * factory_gen : _fac_gens_back) {
        _fac_gens_front.push_back(factory_gen);
    }
    _fac_gens_back.clear();
    _fac_gens_front.swap(_fac_gens_back);

    for (auto * event_proc : _evt_procs_back) {
        _evt_procs_front.push_back(event_proc);
    }
    _evt_procs_back.clear();
    _evt_procs_front.swap(_evt_procs_back);

    for (auto * event_src_gen : _evt_src_gens_back) {
        _evt_src_gens_front.push_back(event_src_gen);
    }
    _evt_src_gens_back.clear();
    _evt_src_gens_front.swap(_evt_src_gens_back);

    for (auto * event_src : _evt_srces_back) {
        _evt_srces_front.push_back(event_src);
    }
    _evt_srces_back.clear();
    _evt_srces_front.swap(_evt_srces_back);
}

/// build_topology() takes all of the components the user has provided up
/// until this point and assembles them in the correct order into a
/// JProcessingTopology object. Ownership of some components will probably
/// get transferred into the JTopology, although this raises some interesting
/// questions.

JProcessingTopology *JTopologyBuilder::build_topology() {

    increase_priority(); // Merges everything into *_back vectors
    JProcessingTopology* topology = new JProcessingTopology(m_app);

    // Add event processors to topology
    for (auto * evt_proc : _evt_procs_back) {
        topology->event_processors.push_back(evt_proc);
        topology->component_summary.event_processors.push_back(
                {.plugin_name="unknown", .type_name="unknown"});
    }

    // Add ready-made event sources to topology
    for (auto * evt_src : _evt_srces_back) {
        topology->event_source_manager.AddJEventSource(evt_src);
        topology->component_summary.event_sources.push_back(
                {.plugin_name = "unknown",
                 .type_name = evt_src->GetType(),
                 .source_name = evt_src->GetName()});
    }

    // Add event source generators to event source manager
    for (auto * event_src_gen : _evt_src_gens_back) {
        topology->event_source_manager.AddJEventSourceGenerator(event_src_gen);
    }

    // Add event source names to event source manager
    for (std::string evt_src_name : _evt_src_names) {
        topology->event_source_manager.AddEventSource(evt_src_name);
    }
    _evt_src_names.clear();

    // Create all event sources
    topology->event_source_manager.CreateSources();

    // Get factory generators from event sources
    std::deque<JEventSource*> sEventSources;
    topology->event_source_manager.GetUnopenedJEventSources(sEventSources);
    std::unordered_set<std::type_index> sSourceTypes;
    // TODO: We aren't actually adding anything to the unordered set
    for(auto sSource : sEventSources)
    {
        auto sTypeIndex = sSource->GetDerivedType();
        if(sSourceTypes.find(sTypeIndex) != std::end(sSourceTypes))
            continue; //same type as before: duplicate factories!

        auto sGenerator = sSource->GetFactoryGenerator();
        if(sGenerator != nullptr) {
            topology->factory_generators.push_back(sGenerator);
        }
    }

    // Add all factory generators to topology
    for (auto factory : _fac_gens_back) {
        topology->factory_generators.push_back(factory);
    }

    // Collect summaries for all factories in factory_generators
    JFactorySet dummy_fac_set(topology->factory_generators);
    topology->component_summary.factories = dummy_fac_set.Summarize();


    size_t event_pool_size = 100;
    size_t event_queue_threshold = 80;
    size_t event_source_chunksize = 40;
    size_t event_processor_chunksize = 1;
    size_t location_count = 1;
    bool enable_stealing = false;
    int affinity = 0;
    int locality = 0;

    auto params = m_app->GetJParameterManager();
    params->SetDefaultParameter("jana:event_pool_size", event_pool_size);
    params->SetDefaultParameter("jana:event_queue_threshold", event_queue_threshold);
    params->SetDefaultParameter("jana:event_source_chunksize", event_source_chunksize);
    params->SetDefaultParameter("jana:event_processor_chunksize", event_processor_chunksize);
    params->SetDefaultParameter("jana:enable_stealing", enable_stealing);
    params->SetDefaultParameter("jana:affinity", affinity);
    params->SetDefaultParameter("jana:locality", locality);

    topology->mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(affinity),
                                 static_cast<JProcessorMapping::LocalityStrategy>(locality));

    topology->event_pool = std::make_shared<JEventPool>(m_app, &topology->factory_generators, event_pool_size, location_count);

    // Assume the simplest possible topology for now, complicate later
    auto queue = new EventQueue(event_queue_threshold, topology->mapping.get_loc_count(), enable_stealing);

    for (auto src : sEventSources) {

        JArrow* arrow = new JEventSourceArrow(src->GetName(), src, queue, topology->event_pool);
        arrow->set_backoff_tries(0);
        topology->arrows.push_back(arrow);
        topology->sources.push_back(arrow);
        arrow->set_chunksize(event_source_chunksize);
        // create arrow for sources. Don't open until arrow.activate() called

        // Add to summary
        topology->component_summary.event_sources.push_back({
            .plugin_name="unknown",
            .type_name=src->GetType(),
            .source_name=src->GetName()
        });
    }

    auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, topology->event_pool);
    proc_arrow->set_chunksize(event_processor_chunksize);
    topology->arrows.push_back(proc_arrow);

    // Receive notifications when sinks finish
    proc_arrow->attach_downstream(topology);
    topology->attach_upstream(proc_arrow);

    for (auto proc :_evt_procs_front) {
        proc_arrow->add_processor(proc);
    }
    for (auto proc :_evt_procs_back) {
        proc_arrow->add_processor(proc);
    }
    topology->sinks.push_back(proc_arrow);


    return topology;
}

JTopologyBuilder::JTopologyBuilder(JApplication* app) : m_app(app) {}

void JTopologyBuilder::print_report() {

    jout << "Event sources: " << std::endl;
    jout << "Event processors: " << std::endl;
    jout << "Event factories: " << std::endl;

}




