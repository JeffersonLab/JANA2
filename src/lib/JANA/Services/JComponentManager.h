//
// Created by Nathan Brei on 2019-08-02.
//

#ifndef JANA2_JCOMPONENTMANAGER_H
#define JANA2_JCOMPONENTMANAGER_H

#include <JANA/JEventSourceGenerator.h>

#include <vector>


class JComponentManager {
public:

    JComponentManager(JApplication*);
    ~JComponentManager();

    void next_plugin(std::string plugin_name);

    void add(std::string event_source_name);
    void add(JEventSourceGenerator* source_generator);
    void add(JFactoryGenerator* factory_generator);
    void add(JEventSource* event_source);
    void add(JEventProcessor* processor);

    void resolve_event_sources();
    JEventSourceGenerator* resolve_user_event_source_generator() const;
    JEventSourceGenerator* resolve_event_source(std::string source_name) const;

    JComponentSummary get_component_summary();

    std::vector<JEventSource*> get_evt_srces();
    std::vector<JEventProcessor*> get_evt_procs();
    std::vector<JFactoryGenerator*> get_fac_gens();


private:
    // Sources need:    { typename, pluginname, srcname, status, evtcnt }
    // Processors need: { typename, pluginname, mutexgroup, status, evtcnt }
    // Factories need:  { typename, pluginname }

    JApplication* m_app;

    std::string m_current_plugin_name;
    std::vector<std::string> m_src_names;
    std::vector<JEventSourceGenerator*> m_src_gens;
    std::vector<JFactoryGenerator*> m_fac_gens;
    std::vector<JEventSource*> m_evt_src_owned;
    std::vector<JEventSource*> m_evt_src_unowned;
    std::vector<JEventProcessor*> m_evt_proc;

    std::string m_user_evt_src_typename = "";
    JEventSourceGenerator* m_user_evt_src_gen = nullptr;

};


#endif //JANA2_JCOMPONENTMANAGER_H
