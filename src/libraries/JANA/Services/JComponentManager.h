
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef JANA2_JCOMPONENTMANAGER_H
#define JANA2_JCOMPONENTMANAGER_H

#include <JANA/JEventSourceGenerator.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Components/JComponentSummary.h>
#include <JANA/Services/JServiceLocator.h>

#include <vector>

class JEventProcessor;
class JEventUnfolder;
class JEventFolder;

class JComponentManager : public JService {
public:

    explicit JComponentManager();
    ~JComponentManager() override;
    void Init() override;

    // Called during plugin loading
    void NextPlugin(std::string plugin_name);

    void Add(std::string event_source_name);
    void Add(JEventSourceGenerator* source_generator);
    void Add(JFactoryGenerator* factory_generator);
    void Add(JEventSource* event_source);
    void Add(JEventProcessor* processor);
    void Add(JEventUnfolder* unfolder);
    void Add(JEventFolder* folder);

    // Called after plugin loading
    void ConfigureComponents();

    // Helpers
    void PreinitializeComponents();
    void ResolveEventSources();
    void InitializeComponents();
    JEventSourceGenerator* ResolveUserEventSourceGenerator() const;
    JEventSourceGenerator* ResolveEventSource(std::string source_name) const;

    // Called after JApplication::Initialize() finishes
    const JComponentSummary& GetComponentSummary();

    // TODO: Deprecate
    std::vector<JEventSourceGenerator*>& get_evt_src_gens();

    std::vector<JEventSourceGenerator*>& GetSourceGenerators();
    std::vector<JEventSource*>& GetSources();
    std::vector<JEventProcessor*>& GetProcessors();
    std::vector<JFactoryGenerator*>& GetFactoryGenerators();
    std::vector<JEventUnfolder*>& GetUnfolders();
    std::vector<JEventFolder*>& GetFolders();

    void ConfigureEvent(JEvent& event);

private:

    Service<JParameterManager> m_params {this};

    std::string m_current_plugin_name;
    std::vector<std::string> m_src_names;
    std::vector<JEventSourceGenerator*> m_src_gens;
    std::vector<JFactoryGenerator*> m_fac_gens;
    std::vector<JEventSource*> m_evt_srces;
    std::vector<JEventProcessor*> m_evt_procs;
    std::vector<JEventUnfolder*> m_unfolders;
    std::vector<JEventFolder*> m_folders;

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
