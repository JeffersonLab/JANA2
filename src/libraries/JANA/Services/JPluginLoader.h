
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JPLUGINLOADER_H
#define JANA2_JPLUGINLOADER_H

#include <JANA/Services/JParameterManager.h>
#include <JANA/JService.h>

#include <string>
#include <vector>
#include <list>
#include <queue>


class JComponentManager;
class JApplication;

class JPlugin {
    friend class JPluginLoader;
    JApplication* m_app;
    JLogger m_logger;
    std::string m_name;
    std::string m_path;
    void* m_handle;
public:
    JPlugin(std::string name, std::string path) : m_name(name), m_path(path) {};
    ~JPlugin();
    std::string GetName() { return m_name; }
    std::string GetPath() { return m_path; }
};

class JPluginLoader : public JService {

public:

    JPluginLoader() {
        SetPrefix("jana");
    }
    ~JPluginLoader() override = default;
    void Init() override;

    void add_plugin(std::string plugin_name);
    void add_plugin_path(std::string path);
    void attach_plugins(JComponentManager* jcm);
    void attach_plugin(std::string name, std::string path);
    void resolve_plugin_paths();

    bool is_valid_plugin_name(const std::string& plugin_name) const;
    bool is_valid_path(const std::string& path) const;
    std::string find_first_valid_path(const std::string& name, std::ostringstream& debug_log) const;
    std::string make_path_from_name(const std::string& name, const std::string& path_prefix) const;

private:
    Service<JParameterManager> m_params {this};

    std::queue<std::string> m_all_plugins_requested;
    std::vector<std::string> m_plugins_from_parameter;
    std::vector<std::string> m_plugins_to_exclude;
    std::vector<std::string> m_plugin_paths;
    std::string m_plugin_paths_str;

    // We wish to preserve each plugin's insertion order
    // This way, plugins are unloaded in the reverse order they were added
    std::list<std::unique_ptr<JPlugin>> m_plugins; 
    std::map<std::string, JPlugin*> m_plugin_index;

    bool m_verbose = false;
};


#endif //JANA2_JPLUGINLOADER_H
