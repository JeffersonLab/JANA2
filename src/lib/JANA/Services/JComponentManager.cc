//
// Created by Nathan Brei on 2019-08-02.
//

#include "JComponentManager.h"
#include <JANA/JEventProcessor.h>

JComponentManager::JComponentManager(JApplication* app) : m_app(app) {}

JComponentManager::~JComponentManager() {
    for (auto* src : m_evt_srces_owned) {
        delete src;
    }
    // TODO: User owns event_procs, but probably won't ever delete them.
    // If the event_proc comes from a plugin, the user _can't_ delete them.
}

void JComponentManager::next_plugin(std::string plugin_name) {
    // We defer resolving event sources until we have finished loading all plugins
    m_current_plugin_name = plugin_name;
}

void JComponentManager::add(std::string event_source_name) {
    m_src_names.push_back(event_source_name);
}

void JComponentManager::add(JEventSourceGenerator *source_generator) {
    source_generator->SetPlugin(m_current_plugin_name);
    source_generator->SetJApplication(m_app);
    m_src_gens.push_back(source_generator);
}

void JComponentManager::add(JFactoryGenerator *factory_generator) {
    m_fac_gens.push_back(factory_generator);
}

void JComponentManager::add(JAbstractEventSource *event_source) {
    event_source->SetPluginName(m_current_plugin_name);
    event_source->SetApplication(m_app);
    m_evt_srces_all.push_back(event_source);
}

void JComponentManager::add(JAbstractEventProcessor *processor) {
    processor->SetPluginName(m_current_plugin_name);
    if (processor->GetApplication() == nullptr) {
        processor->SetJApplication(m_app);
    }
    m_evt_procs.push_back(processor);
}

void JComponentManager::resolve_event_sources() {

    m_app->SetDefaultParameter("event_source_type", m_user_evt_src_typename, "");

    m_user_evt_src_gen = resolve_user_event_source_generator();
    for (auto& source_name : m_src_names) {
        auto* generator = resolve_event_source(source_name);
        auto source = generator->MakeJEventSource(source_name);
        source->SetPluginName(generator->GetPlugin());
        source->SetApplication(m_app);
        auto fac_gen = source->GetFactoryGenerator();
        if (fac_gen != nullptr) {
            m_fac_gens.push_back(source->GetFactoryGenerator());
        }
        m_evt_srces_all.push_back(source);
        m_evt_srces_owned.push_back(source);
    }

    m_app->SetDefaultParameter("jana:nevents", m_nevents, "Max number of events that sources can emit");
    m_app->SetDefaultParameter("jana:nskip", m_nskip, "Number of events that sources should skip before starting emitting");

    for (auto source : m_evt_srces_all) {
        source->SetRange(m_nskip, m_nevents);
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

std::vector<JAbstractEventSource*>& JComponentManager::get_evt_srces() {
    return m_evt_srces_all;
}

std::vector<JAbstractEventProcessor*>& JComponentManager::get_evt_procs() {
    return m_evt_procs;
}

std::vector<JFactoryGenerator*>& JComponentManager::get_fac_gens() {
    return m_fac_gens;
}

JComponentSummary JComponentManager::get_component_summary() {
    JComponentSummary result;

    // Event sources
    for (auto * src : m_evt_srces_all) {
        result.event_sources.push_back({.plugin_name=src->GetPluginName(), .type_name=src->GetType(), .source_name=src->GetName()});
    }

    // Event processors
    for (auto * evt_proc : m_evt_procs) {
        result.event_processors.push_back({.plugin_name = evt_proc->GetPluginName(), .type_name=evt_proc->GetTypeName()});
    }

    // Factories
    JFactorySet dummy_fac_set(m_fac_gens);
    result.factories = dummy_fac_set.Summarize();

    return result;
}

