
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JPluginLoader.h"
#include "JComponentManager.h"
#include "JLoggingService.h"
#include "JParameterManager.h"

#include <dlfcn.h>
#include <iostream>
#include <unistd.h>
#include <set>

class JApplication;

void JPluginLoader::add_plugin(std::string plugin_name) {
    /// Add the specified plugin to the list of plugins to be
    /// attached. This only records the name. The plugin is not
    /// actually attached until AttachPlugins() is called (typically
    /// from Initialize() which is called from Run()).
    /// This will check if the plugin already exists in the list
    /// of plugins to attach and will not add it a second time
    /// if it is already there. This may be important if the order
    /// of plugins is important. It is left to the user to handle
    /// in those cases.
    ///
    /// @param plugin_name name of the plugin. Do not include the
    ///                    ".so" or ".dylib" suffix in the name.
    ///                    The path to the plugin will be searched
    ///                    from the JANA_PLUGIN_PATH envar.
    ///
    for (std::string& n : m_plugins_to_include) {
        if (n == plugin_name) {
            return;
        }
    }
    m_plugins_to_include.push_back(plugin_name);
}


void JPluginLoader::add_plugin_path(std::string path) {

    /// Add a path to the directories searched for plugins. This
    /// should not include the plugin name itself. This only has
    /// an effect when called before AttachPlugins is called
    /// (i.e. before Run is called).
    /// n.b. if this is called with a path already in the list,
    /// then the call is silently ignored.
    ///
    /// Generally, users will set the path via the JANA_PLUGIN_PATH
    /// environment variable and won't need to call this method. This
    /// may be called if it needs to be done programmatically.
    ///
    /// @param path directory to search for plugins.
    ///
    for (std::string& n : m_plugin_paths) {
        if (n == path) {
            return;
        }
    }
    m_plugin_paths.push_back(path);
}


void JPluginLoader::attach_plugins(JComponentManager* jcm) {
    /// Loop over list of plugin names added via AddPlugin() and
    /// actually attach and initialize them. See AddPlugin method
    /// for more.

    // Build our list of plugin search paths.
    // 1. First we look for plugins in the local directory
    add_plugin_path(".");

    // 2. Next we look for plugins in locations specified via parameters. (Colon-separated)
    std::stringstream param_ss(m_plugin_paths_str);
    std::string path;
    while (getline(param_ss, path, ':')) add_plugin_path(path);

    // 3. Next we look for plugins in locations specified via environment variable. (Colon-separated)
    const char* jpp = getenv("JANA_PLUGIN_PATH");
    if (jpp) {
        std::stringstream envvar_ss(jpp);
        while (getline(envvar_ss, path, ':')) add_plugin_path(path);
    }

    // 4. Finally we look in the plugin directories relative to $JANA_HOME
    if (const char* jana_home = getenv("JANA_HOME")) {
        add_plugin_path(std::string(jana_home) + "/plugins/JANA");  // In case we did a system install and want to avoid conflicts.
        add_plugin_path(std::string(jana_home) + "/plugins");
    }

    // Add plugins specified via PLUGINS configuration parameter
    // (comma separated list).
    std::set<std::string> exclusions(m_plugins_to_exclude.begin(), m_plugins_to_exclude.end());

    // Loop over plugins
    // It is possible for plugins to add additional plugins that will also need to
    // be attached. To accommodate this we wrap the following chunk of code in
    // a lambda function so we can run it over the additional plugins recursively
    // until all are attached. (see below)
    auto add_plugins_lamda = [=, this](std::vector<std::string> &plugins) {
        std::stringstream paths_checked;
        for (const std::string& plugin : plugins) {
            // The user might provide a short name like "JTest", or a long name like "JTest.so".
            // We assume that the plugin extension is always ".so". This may pose a problem on macOS
            // where the extension might default to ".dylib".
            std::string plugin_shortname;
            std::string plugin_fullname;
            if (plugin.substr(plugin.size() - 3) != ".so") {
                plugin_fullname = plugin + ".so";
                plugin_shortname = plugin;
            }
            else {
                plugin_fullname = plugin;
                plugin_shortname = plugin.substr(0, plugin.size()-3);
            }
            if (exclusions.find(plugin_shortname) != exclusions.end() ||
                 exclusions.find(plugin_fullname) != exclusions.end()) {

                LOG_INFO(m_logger) << "Excluding plugin `" << plugin << "`" << LOG_END;
                continue;
            }

            // Loop over paths
            bool found_plugin = false;
            for (std::string path : m_plugin_paths) {
                std::string fullpath = path + "/" + plugin_fullname;
                LOG_DEBUG(m_logger) << "Looking for '" << fullpath << "' ...." << LOG_END;
                paths_checked << "    " << fullpath << "  =>  ";
                if (access(fullpath.c_str(), F_OK) != -1) {
                    LOG_DEBUG(m_logger) << "Found!" << LOG_END;
                    try {
                        jcm->next_plugin(plugin_shortname);
                        attach_plugin(fullpath.c_str());
                        paths_checked << "Loaded successfully" << std::endl;
                        found_plugin = true;
                        break;
                    } catch (...) {
                        paths_checked << "Loading failure: " << dlerror() << std::endl;
                        LOG_WARN(m_logger) << "Loading failure: " << dlerror() << LOG_END;
                        continue;
                    }
                }
                paths_checked << "File not found" << std::endl;
                LOG_DEBUG(m_logger) << "Failed to attach '" << fullpath << "'" << LOG_END;
            }

            // If we didn't find the plugin, then complain and quit
            if (!found_plugin) {
                LOG_ERROR(m_logger) << "Couldn't load plugin '" << plugin << "'\n" <<
                                    "  Make sure that JANA_HOME and/or JANA_PLUGIN_PATH environment variables are set correctly.\n"
                                    <<
                                    "  Paths checked:\n" << paths_checked.str() << LOG_END;
                throw JException("Couldn't find plugin '%s'", plugin.c_str());
            }
        }
    };

    // Recursively loop over the list of plugins to ensure new plugins added by ones being
    // attached are also attached.
    uint64_t inext = 0;
    while(inext < m_plugins_to_include.size() ){
        std::vector<std::string> myplugins(m_plugins_to_include.begin() + inext, m_plugins_to_include.end());
        inext = m_plugins_to_include.size(); // new plugins will be attached to end of vector
        add_plugins_lamda(myplugins);
    }
}


void JPluginLoader::attach_plugin(std::string soname) {

    /// Attach a plugin by opening the shared object file and running the
    /// InitPlugin_t(JApplication* app) global C-style routine in it.
    /// An exception will be thrown if the plugin is not successfully opened.
    /// Users will not need to call this directly since it is called automatically
    /// from Initialize().
    ///
    /// @param soname name of shared object file to attach. This may include
    ///               an absolute or relative path.
    ///
    /// @param verbose if set to true, failed attempts will be recorded via the
    ///                JLog. Default is false so JANA can silently ignore files
    ///                that are not valid plugins.
    ///

    // Open shared object
    void* handle = dlopen(soname.c_str(), RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE);
    if (!handle) {
        LOG_DEBUG(m_logger) << dlerror() << LOG_END;
        throw "dlopen failed";
    }

    // Look for an InitPlugin symbol
    typedef void InitPlugin_t(JApplication* app);
    InitPlugin_t* initialize_proc = (InitPlugin_t*) dlsym(handle, "InitPlugin");
    if (initialize_proc) {
        LOG_INFO(m_logger) << "Initializing plugin \"" << soname << "\"" << LOG_END;
        (*initialize_proc)(m_app);
        m_sohandles[soname] = handle;
    } else {
        dlclose(handle);
        LOG_DEBUG(m_logger) << "Plugin \"" << soname
                            << "\" does not have an InitPlugin() function. Ignoring." << LOG_END;
    }
}


JPluginLoader::JPluginLoader(JApplication* app) : m_app(app) {}

JPluginLoader::~JPluginLoader(){

    // Loop over open plugin handles.
    // Call FinalizePlugin if it has one and close it in all cases.
    typedef void FinalizePlugin_t(JApplication* app);
    for( auto p :m_sohandles ){
        auto soname = p.first;
        auto handle = p.second;
        FinalizePlugin_t* finalize_proc = (FinalizePlugin_t*) dlsym(handle, "FinalizePlugin");
        if (finalize_proc) {
            LOG_INFO(m_logger) << "Finalizing plugin \"" << soname << "\"" << LOG_END;
            (*finalize_proc)(m_app);
        }

        // Close plugin handle
        dlclose(handle);
    }
}

void JPluginLoader::acquire_services(JServiceLocator* sl) {

    auto params = sl->get<JParameterManager>();
    params->SetDefaultParameter("plugins", m_plugins_to_include, "Comma-separated list of plugins to load.");
    params->SetDefaultParameter("plugins_to_ignore", m_plugins_to_exclude, "Comma-separated list of plugins to NOT load, even if they are specified in 'plugins'.");
    m_app->SetDefaultParameter("jana:plugin_path", m_plugin_paths_str, "Colon-separated list of paths to search for plugins");
    params->SetDefaultParameter("jana:debug_plugin_loading", m_verbose, "Trace the plugin search path and display any loading errors");

    m_logger = sl->get<JLoggingService>()->get_logger("JPluginLoader");
    if (m_verbose) {
        // The jana:debug_plugin_loading parameter is kept around for backwards compatibility
        // at least for now
        m_logger.level = JLogger::Level::TRACE;
    }
}



