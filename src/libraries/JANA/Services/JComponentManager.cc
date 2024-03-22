
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JComponentManager.h"
#include <JANA/JEventProcessor.h>
#include <JANA/JFactoryGenerator.h>
#include <JANA/JEventUnfolder.h>

JComponentManager::JComponentManager(JApplication* app) : m_app(app) {
}

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

void JComponentManager::next_plugin(std::string plugin_name) {
    // We defer resolving event sources until we have finished loading all plugins
    m_current_plugin_name = plugin_name;
}

void JComponentManager::add(std::string event_source_name) {
    m_src_names.push_back(event_source_name);
}

void JComponentManager::add(JEventSourceGenerator *source_generator) {
    source_generator->SetPluginName(m_current_plugin_name);
    source_generator->SetJApplication(m_app);
    m_src_gens.push_back(source_generator);
}

void JComponentManager::add(JFactoryGenerator *factory_generator) {
    factory_generator->SetPluginName(m_current_plugin_name);
    factory_generator->SetApplication(m_app);
    m_fac_gens.push_back(factory_generator);
}

void JComponentManager::add(JEventSource *event_source) {
    event_source->SetPluginName(m_current_plugin_name);
    event_source->SetApplication(m_app);
    m_evt_srces.push_back(event_source);
    auto fac_gen = event_source->GetFactoryGenerator();
    if (fac_gen != nullptr) {
        fac_gen->SetPluginName(m_current_plugin_name);
        fac_gen->SetApplication(m_app);
        m_fac_gens.push_back(fac_gen);
    }
}

void JComponentManager::add(JEventProcessor *processor) {
    processor->SetPluginName(m_current_plugin_name);
    processor->SetApplication(m_app);
    m_evt_procs.push_back(processor);
}

void JComponentManager::add(JEventUnfolder* unfolder) {
    unfolder->SetPluginName(m_current_plugin_name);
    unfolder->SetApplication(m_app);
    m_unfolders.push_back(unfolder);
}

void JComponentManager::configure_event(JEvent& event) {
    auto factory_set = new JFactorySet(m_fac_gens);
    event.SetFactorySet(factory_set);
    event.SetDefaultTags(m_default_tags);
    event.GetJCallGraphRecorder()->SetEnabled(m_enable_call_graph_recording);
}

void JComponentManager::initialize() {
    // We want to obtain parameters from here rather than in the constructor.
    // If we set them here, plugins and test cases can set parameters right up until JApplication::Initialize()
    // or Run() are called. Otherwise, the parameters have to be set before the
    // JApplication is even constructed.
    auto parms = m_app->GetJParameterManager();
    parms->SetDefaultParameter("record_call_stack", m_enable_call_graph_recording, "Records a trace of who called each factory. Reduces performance but necessary for plugins such as janadot.");
    parms->FilterParameters(m_default_tags, "DEFTAG:");
}


void JComponentManager::resolve_event_sources() {

    m_app->SetDefaultParameter("event_source_type", m_user_evt_src_typename, "Manually specifies which JEventSource should open the input file");

    m_user_evt_src_gen = resolve_user_event_source_generator();
    for (auto& source_name : m_src_names) {
        auto* generator = resolve_event_source(source_name);
        auto source = generator->MakeJEventSource(source_name);
        source->SetPluginName(generator->GetPluginName());
        source->SetApplication(m_app);
        auto fac_gen = source->GetFactoryGenerator();
        if (fac_gen != nullptr) {
            fac_gen->SetPluginName(m_current_plugin_name);
            fac_gen->SetApplication(m_app);
            m_fac_gens.push_back(fac_gen);
        }
        m_evt_srces.push_back(source);
    }

    m_app->SetDefaultParameter("jana:nevents", m_nevents, "Max number of events that sources can emit");
    m_app->SetDefaultParameter("jana:nskip", m_nskip, "Number of events that sources should skip before starting emitting");

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

JComponentSummary JComponentManager::get_component_summary() {
    JComponentSummary result;

    // Event sources
    for (auto * src : m_evt_srces) {
        result.event_sources.push_back({.level=src->GetLevel(), .plugin_name=src->GetPluginName(), .type_name=src->GetType(), .source_name=src->GetName()});
    }

    // Event processors
    for (auto * evt_proc : m_evt_procs) {
        result.event_processors.push_back({.level=evt_proc->GetLevel(), .plugin_name = evt_proc->GetPluginName(), .type_name=evt_proc->GetTypeName()});
    }

    for (auto * unfolder : m_unfolders) {
        result.event_unfolders.push_back({.level=unfolder->GetLevel(), .plugin_name = unfolder->GetPluginName(), .type_name=unfolder->GetTypeName()});
    }

    // Factories
    JFactorySet dummy_fac_set(m_fac_gens);
    result.factories = dummy_fac_set.Summarize();

    return result;
}

