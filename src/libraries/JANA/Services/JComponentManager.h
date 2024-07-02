
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JCOMPONENTMANAGER_H
#define JANA2_JCOMPONENTMANAGER_H

#include <JANA/JEventSourceGenerator.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Status/JComponentSummary.h>
#include <JANA/Services/JServiceLocator.h>

#include <vector>

class JEventProcessor;
class JEventUnfolder;

class JComponentManager : public JService {
public:

    explicit JComponentManager();
    ~JComponentManager() override;
    void Init() override;

    void next_plugin(std::string plugin_name);

    void add(std::string event_source_name);
    void add(JEventSourceGenerator* source_generator);
    void add(JFactoryGenerator* factory_generator);
    void add(JEventSource* event_source);
    void add(JEventProcessor* processor);
    void add(JEventUnfolder* unfolder);

    void preinitialize_components();
    void resolve_event_sources();
    void initialize_components();
    JEventSourceGenerator* resolve_user_event_source_generator() const;
    JEventSourceGenerator* resolve_event_source(std::string source_name) const;

    const JComponentSummary& get_component_summary();

    // Unsafe access into our own repository of components
    std::vector<JEventSourceGenerator*>& get_evt_src_gens();
    std::vector<JEventSource*>& get_evt_srces();
    std::vector<JEventProcessor*>& get_evt_procs();
    std::vector<JFactoryGenerator*>& get_fac_gens();
    std::vector<JEventUnfolder*>& get_unfolders();

    void configure_event(JEvent& event);

private:
    // Sources need:    { typename, pluginname, srcname, status, evtcnt }
    // Processors need: { typename, pluginname, mutexgroup, status, evtcnt }
    // Factories need:  { typename, pluginname }

    Service<JParameterManager> m_params {this};
    Service<JLoggingService> m_logging {this};

    std::string m_current_plugin_name;
    std::vector<std::string> m_src_names;
    std::vector<JEventSourceGenerator*> m_src_gens;
    std::vector<JFactoryGenerator*> m_fac_gens;
    std::vector<JEventSource*> m_evt_srces;
    std::vector<JEventProcessor*> m_evt_procs;
    std::vector<JEventUnfolder*> m_unfolders;

    std::map<std::string, std::string> m_default_tags;
    bool m_enable_call_graph_recording = false;
    std::string m_autoactivate;

    uint64_t m_nskip=0;
    uint64_t m_nevents=0;
    std::string m_user_evt_src_typename = "";
    JEventSourceGenerator* m_user_evt_src_gen = nullptr;

    JComponentSummary m_summary;
};


#endif //JANA2_JCOMPONENTMANAGER_H
