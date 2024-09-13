
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JComponentManager.h"
#include <JANA/JEventProcessor.h>
#include <JANA/JMultifactory.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JEventUnfolder.h>
#include <JANA/Utils/JAutoActivator.h>

JComponentManager::JComponentManager() {}

JComponentManager::~JComponentManager() {

    for (auto* src : m_evt_srces) {
        delete src;
    }
    for (auto* proc : m_evt_procs) {
        delete proc;
    }
    for (auto* fac_gen : m_fac_gens) {
        delete fac_gen;
    }
    for (auto* src_gen : m_src_gens) {
        delete src_gen;
    }
    for (auto* unfolder : m_unfolders) {
        delete unfolder;
    }
}

void JComponentManager::Init() {
    // Because this is an 'internal' JService, it works a little differently than user-provided
    // JServices. Specifically, we have to fetch parameters _after_ plugin loading because some 
    // plugins want to set parameter values that JComponentManager needs (most notably janadot). 
    // We handle parameters in configure_components() instead.
}

void JComponentManager::configure_components() {

    m_params->SetDefaultParameter("event_source_type", m_user_evt_src_typename, "Manually specifies which JEventSource should open the input file");
    m_params->SetDefaultParameter("record_call_stack", 
                                  m_enable_call_graph_recording,
                                  "Records a trace of who called each factory. Reduces performance but necessary for plugins such as janadot.")
            ->SetIsAdvanced(true);
    m_params->SetDefaultParameter("jana:nevents", m_nevents, "Max number of events that sources can emit");
    m_params->SetDefaultParameter("jana:nskip", m_nskip, "Number of events that sources should skip before starting emitting");
    m_params->SetDefaultParameter("autoactivate", m_autoactivate, "List of factories to activate regardless of what the event processors request. Format is typename:tag,typename:tag");
    m_params->FilterParameters(m_default_tags, "DEFTAG:");

    // Look for factories to auto-activate
    if (!m_autoactivate.empty()) {
        add(new JAutoActivator);
        // JAutoActivator will re-parse the autoactivate list by itself
    }

    // Give all components a JApplication pointer and a logger
    preinitialize_components();

    // Resolve all event sources now that all plugins have been loaded
    resolve_event_sources();

    // Call Summarize() and Init() in order to populate JComponentSummary and JParameterManager, respectively
    initialize_components();
}

void JComponentManager::preinitialize_components() {
    for (auto* src : m_evt_srces) {
        src->SetApplication(GetApplication());
        src->SetLogger(m_params->GetLogger(src->GetLoggerName()));
    }
    for (auto* proc : m_evt_procs) {
        proc->SetApplication(GetApplication());
        proc->SetLogger(m_params->GetLogger(proc->GetLoggerName()));
    }
    for (auto* fac_gen : m_fac_gens) {
        fac_gen->SetApplication(GetApplication());
        //fac_gen->SetLogger(m_logging->get_logger(fac_gen->GetLoggerName()));
    }
    for (auto* src_gen : m_src_gens) {
        src_gen->SetJApplication(GetApplication());
        //src_gen->SetLogger(m_logging->get_logger(src_gen->GetLoggerName()));
    }
    for (auto* unfolder : m_unfolders) {
        unfolder->SetApplication(GetApplication());
        unfolder->SetLogger(m_params->GetLogger(unfolder->GetLoggerName()));
    }
}

void JComponentManager::initialize_components() {
    // For now, this only computes the summary for all components except factories.
    // However, we are likely to eventually want summaries to access information only
    // available after component initialization, specifically parameters. In this case, 
    // we would move responsibility for source, unfolder, and processor initialization 
    // out from the JArrowTopology and into here.

    // Event sources
    for (auto * src : m_evt_srces) {
        src->Summarize(m_summary);
    }

    // Event processors
    for (auto * evt_proc : m_evt_procs) {
        evt_proc->Summarize(m_summary);
    }

    // Unfolders
    for (auto * unfolder : m_unfolders) {
        unfolder->Summarize(m_summary);
    }

    JFactorySet dummy_fac_set(m_fac_gens);

    // Factories
    for (auto* fac : dummy_fac_set.GetAllFactories()) {
        try {
            // Run Init() on each factory in order to capture any parameters 
            // (and eventually services) that are retrieved via GetApplication().
            fac->DoInit();
        }
        catch (...) {
            // Swallow any exceptions!
            // Everything in dummy_fac_set will be destroyed immediately after this.
            // The same exception will be thrown from a fresh factory
            // set once processing begins, unless the factory is never used.
        }
        fac->Summarize(m_summary);
    }

    // Multifactories
    for (auto* fac : dummy_fac_set.GetAllMultifactories()) {
        try {
            fac->DoInit();
        }
        catch (...) {
            // Swallow any exceptions! See above.
        }
        fac->Summarize(m_summary);
    }
}

void JComponentManager::next_plugin(std::string plugin_name) {
    // We defer resolving event sources until we have finished loading all plugins
    m_current_plugin_name = plugin_name;
}

void JComponentManager::add(std::string event_source_name) {
    m_src_names.push_back(event_source_name);
}

void JComponentManager::add(JEventSourceGenerator *source_generator) {
    source_generator->SetPluginName(m_current_plugin_name);
    m_src_gens.push_back(source_generator);
}

void JComponentManager::add(JFactoryGenerator *factory_generator) {
    factory_generator->SetPluginName(m_current_plugin_name);
    m_fac_gens.push_back(factory_generator);
}

void JComponentManager::add(JEventSource *event_source) {
    event_source->SetPluginName(m_current_plugin_name);
    m_evt_srces.push_back(event_source);
}

void JComponentManager::add(JEventProcessor *processor) {
    processor->SetPluginName(m_current_plugin_name);
    m_evt_procs.push_back(processor);
}

void JComponentManager::add(JEventUnfolder* unfolder) {
    unfolder->SetPluginName(m_current_plugin_name);
    m_unfolders.push_back(unfolder);
}

void JComponentManager::configure_event(JEvent& event) {
    auto factory_set = new JFactorySet(m_fac_gens);
    event.SetFactorySet(factory_set);
    event.SetDefaultTags(m_default_tags);
    event.GetJCallGraphRecorder()->SetEnabled(m_enable_call_graph_recording);
    event.SetJApplication(GetApplication());
}



void JComponentManager::resolve_event_sources() {

    m_user_evt_src_gen = resolve_user_event_source_generator();
    for (auto& source_name : m_src_names) {
        auto* generator = resolve_event_source(source_name);
        auto source = generator->MakeJEventSource(source_name);
        source->SetPluginName(generator->GetPluginName());
        source->SetApplication(GetApplication());
        m_evt_srces.push_back(source);
    }

    for (auto source : m_evt_srces) {
        // If nskip/nevents are set individually on JEventSources, respect those. Otherwise use global values.
        // Note that this is not what we usually want when we have multiple event sources. It would make more sense to
        // take the nskip/nevent slice across the stream of events emitted by each JEventSource in turn.
        if (source->GetNSkip() == 0) source->SetNSkip(m_nskip);
        if (source->GetNEvents() == 0) source->SetNEvents(m_nevents);
    }
}

JEventSourceGenerator *JComponentManager::resolve_event_source(std::string source_name) const {

    // Always use the user override if they provided one
    if (m_user_evt_src_gen != nullptr) {
        return m_user_evt_src_gen;
    }

    // Otherwise determine the best via CheckOpenable()
    JEventSourceGenerator* best_gen = nullptr;
    double best_likelihood = 0.0;
    for (auto* gen : m_src_gens) {
        auto likelihood = gen->CheckOpenable(source_name);
        if (best_likelihood < likelihood) {
            best_likelihood = likelihood;
            best_gen = gen;
        }
    }

    // If we found the best, use that
    if (best_gen != nullptr) {
        return best_gen;
    }

    // Otherwise, report the problem and throw
    auto ex = JException("Unable to open event source \"%s\": No suitable generator found!", source_name.c_str());
    std::ostringstream os;
    make_backtrace(os);
    ex.stacktrace = os.str();
    throw ex;
}


JEventSourceGenerator* JComponentManager::resolve_user_event_source_generator() const {

    // If the user didn't specify an EVENT_SOURCE_TYPE, do nothing
    if (m_user_evt_src_typename == "") return nullptr;

    // Attempt to find the user event source generator
    for (auto* evt_src_gen : m_src_gens) {
        if (evt_src_gen->GetType() == m_user_evt_src_typename) {
            // Found! Save and exit
            return evt_src_gen;
        }
    }

    // No user event source generator found; generate a message and throw
    std::ostringstream os;
    os << "You specified event source type \"" << m_user_evt_src_typename << "\"" << std::endl;
    os << "be used to read the event sources but no such type exists." << std::endl;
    os << "Here is a list of available source types:" << std::endl << std::endl;
    for (auto * evt_src_gen : m_src_gens) {
        os << "    " << evt_src_gen->GetType() << std::endl;
    }
    os << std::endl;
    throw JException(os.str());

}

std::vector<JEventSourceGenerator*>& JComponentManager::get_evt_src_gens() {
    return m_src_gens;
}

std::vector<JEventSource*>& JComponentManager::get_evt_srces() {
    return m_evt_srces;
}

std::vector<JEventProcessor*>& JComponentManager::get_evt_procs() {
    return m_evt_procs;
}

std::vector<JFactoryGenerator*>& JComponentManager::get_fac_gens() {
    return m_fac_gens;
}

std::vector<JEventUnfolder*>& JComponentManager::get_unfolders() {
    return m_unfolders;
}

const JComponentSummary& JComponentManager::get_component_summary() {
    return m_summary;
}

