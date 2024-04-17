
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JPLUGINLOADER_H
#define JANA2_JPLUGINLOADER_H

#include <JANA/Services/JParameterManager.h>
#include <JANA/JService.h>

#include <string>
#include <vector>


class JComponentManager;
class JApplication;

class JPluginLoader : public JService {

public:

    JPluginLoader();
    ~JPluginLoader() override;
    void Init() override;

    void add_plugin(std::string plugin_name);
    void add_plugin_path(std::string path);
    void attach_plugins(JComponentManager* jcm);
    void attach_plugin(std::string plugin_name);

private:
    Service<JParameterManager> m_params {this};

    std::vector<std::string> m_plugins_to_include;
    std::vector<std::string> m_plugins_to_exclude;
    std::vector<std::string> m_plugin_paths;
    std::string m_plugin_paths_str;
    std::map<std::string, void*> m_sohandles; // key=plugin name  val=dlopen handle

    bool m_verbose = false;

};


#endif //JANA2_JPLUGINLOADER_H
