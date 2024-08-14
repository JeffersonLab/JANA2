
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JPluginLoader.h"
#include "JComponentManager.h"
#include "JParameterManager.h"
#include <JANA/JVersion.h>

#include <cstdlib>
#include <dlfcn.h>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <set>
#include <filesystem>
#include <memory>

class JApplication;

void JPluginLoader::Init() {

    m_params->SetDefaultParameter("plugins", m_plugins_to_include, "Comma-separated list of plugins to load.");
    m_params->SetDefaultParameter("plugins_to_ignore", m_plugins_to_exclude, "Comma-separated list of plugins to NOT load, even if they are specified in 'plugins'.");
    m_params->SetDefaultParameter("jana:plugin_path", m_plugin_paths_str, "Colon-separated list of paths to search for plugins");
    m_params->SetDefaultParameter("jana:debug_plugin_loading", m_verbose, "Trace the plugin search path and display any loading errors");

    if (m_verbose) {
        // The jana:debug_plugin_loading parameter is kept around for backwards compatibility
        // at least for now
        GetLogger().level = JLogger::Level::TRACE;
    }
}



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

void JPluginLoader::resolve_plugin_paths() {
    // Build our list of plugin search paths.
 
    // 1. First we look for plugins in locations specified via parameters. (Colon-separated)
    std::stringstream param_ss(m_plugin_paths_str);
    std::string path;
    while (getline(param_ss, path, ':')) add_plugin_path(path);

    // 2. Next we look for plugins in locations specified via environment variable. (Colon-separated)
    if (const char* jpp = getenv("JANA_PLUGIN_PATH")) {
        std::stringstream envvar_ss(jpp);
        while (getline(envvar_ss, path, ':')) add_plugin_path(path);
    }

    // 3. Next we look in the plugin directories relative to $JANA_HOME
    if (const char* jana_home = getenv("JANA_HOME")) {
        add_plugin_path(std::string(jana_home) + "/lib/JANA/plugins");  // In case we did a system install and want to avoid conflicts.
    }

    // 4. Finally we look in the JANA install directory.
    // By checking here, the user no longer needs to set JANA_HOME in order to run built-in plugins 
    // such as janadot and JTest. The install directory is supposed to be the same as JANA_HOME, 
    // but we can't guarantee that because the user can set JANA_HOME to be anything they want.
    // It would be nice if nothing in the JANA codebase itself relied on JANA_HOME, although we 
    // won't be removing it anytime soon because of build_scripts.
    add_plugin_path(JVersion::GetInstallDir() + "/lib/JANA/plugins");
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
    
    // Figure out the search paths for plugins. For now, plugins _cannot_ add to the search paths
    resolve_plugin_paths();

    // Figure out the set of plugins to exclude. Note that this applies to plugins added
    // by other plugins as well.
    std::set<std::string> exclusions(m_plugins_to_exclude.begin(), m_plugins_to_exclude.end());

    // Loop over all requested plugins as per the `plugins` parameter. Note that this vector may grow as 
    // plugins themselves request additional plugins. These get appended to the back of `m_plugins_to_include`.
    for (int i=0; i<m_plugins_to_include.size(); ++i) {
        std::ostringstream paths_checked;
        const std::string& user_plugin = m_plugins_to_include[i];

        std::string name;
        bool user_provided_path = is_path(user_plugin);
        if (user_provided_path) {
            name = extract_name_from_path(user_plugin);
        }
        else {
            name = normalize_name(user_plugin);
        }

        if (exclusions.find(name) != exclusions.end() || exclusions.find(name) != exclusions.end()) {
            LOG_INFO(m_logger) << "Excluding plugin `" << name << "`" << LOG_END;
            continue;
        }

        std::string path;
        if (user_provided_path) {
            if(validate_path(user_plugin)) {
                paths_checked << "    " << user_plugin << " => Found" << std::endl;
                path = user_plugin;
            }
            else {
                // User entered an invalid path; `path` variable stays enpty
                paths_checked << "    " << user_plugin << " => Not found" << std::endl;
            }
        }
        else {
            path = find_first_valid_path(name, paths_checked);
            // User didn't provide a path, so we have to search
            // If no valid paths found, `path` variable stays empty
        }

        if (path.empty()) {

            LOG_ERROR(m_logger) << "Couldn't find plugin '" << name << "'\n" <<
                                "  Make sure that JANA_HOME and/or JANA_PLUGIN_PATH environment variables are set correctly.\n"
                                <<
                                "  Paths checked:\n" << paths_checked.str() << LOG_END;
            throw JException("Couldn't find plugin '%s'", name.c_str());
        }

        // At this point, the plugin has been found, and we are going to try to attach it. 
        // If the attachment fails for any reason, so does attach_plugins() and ultimately JApplication::Initialize().
        // We do not attempt to search for non-failing plugins by looking further down the search path, because this 
        // masks the "root cause" error and causes much greater confusion. A specific example is working in an environment
        // that has a read-only system install of JANA, e.g. Singularity or CVMFS. If the host application is built using
        // a newer version of JANA which is not binary-compatible with the system install, and both locations end up on the 
        // plugin search path, a legitimate error loading the correct plugin would be suppressed, and the user would instead
        // see an extremely difficult-to-debug error (usually a segfault) stemming from the binary incompatibility 
        // between the host application and the plugin.

        jcm->next_plugin(name);
        attach_plugin(name, path); // Throws JException on failure
    }
}


void JPluginLoader::attach_plugin(std::string name, std::string path) {

    /// Attach a plugin by opening the shared object file and running the
    /// InitPlugin_t(JApplication* app) global C-style routine in it.
    /// An exception will be thrown if the plugin is not successfully opened.
    /// Users will not need to call this directly since it is called automatically
    /// from Initialize().

    // Open shared object
    dlerror(); // Clear any earlier dlerrors
    void* handle = dlopen(path.c_str(), RTLD_LAZY | RTLD_GLOBAL | RTLD_NODELETE);
    if (!handle) {
        std::string err = dlerror();
        LOG_ERROR(m_logger) << "Plugin \"" << name << "\" dlopen() failed: " << err << LOG_END;
        throw JException("Plugin '%s' dlopen() failed: %s", name.c_str(), err.c_str());
    }

    // Retrieve the InitPlugin symbol
    typedef void InitPlugin_t(JApplication* app);
    InitPlugin_t* initialize_proc = (InitPlugin_t*) dlsym(handle, "InitPlugin");
    if (!initialize_proc) {
        dlclose(handle);
        LOG_ERROR(m_logger) << "Plugin \"" << name << "\" is missing 'InitPlugin' symbol" << LOG_END;
        throw JException("Plugin '%s' is missing 'InitPlugin' symbol", name.c_str());
    }

    // Run InitPlugin() and wrap exceptions as needed
    LOG_INFO(m_logger) << "Initializing plugin \"" << name << "\" at path \"" << path << "\"" << LOG_END;

    try {
        (*initialize_proc)(GetApplication());
    }
    catch (JException& ex) {
        if (ex.function_name.empty()) ex.function_name = "attach_plugin";
        if (ex.type_name.empty()) ex.type_name = "JPluginLoader";
        if (ex.instance_name.empty()) ex.instance_name = m_prefix;
        if (ex.plugin_name.empty()) ex.plugin_name = name;
        throw ex;
    }
    catch (std::exception& e) {
        auto ex = JException(e.what());
        ex.exception_type = JTypeInfo::demangle_current_exception_type();
        ex.nested_exception = std::current_exception();
        ex.function_name = "attach_plugin";
        ex.type_name = "JPluginLoader";
        ex.instance_name = m_prefix;
        ex.plugin_name = name;
        throw ex;
    }
    catch (...) {
        auto ex = JException("Unknown exception");
        ex.exception_type = JTypeInfo::demangle_current_exception_type();
        ex.nested_exception = std::current_exception();
        ex.function_name = "attach_plugin";
        ex.type_name = "JPluginLoader";
        ex.instance_name = m_prefix;
        ex.plugin_name = name;
        throw ex;
    }

    // Do some bookkeeping
    auto plugin = std::make_unique<JPlugin>(name, path);
    plugin->m_app = GetApplication();
    plugin->m_logger = m_logger;
    plugin->m_handle = handle;
    m_plugin_index[name] = plugin.get();
    m_plugins.push_front(std::move(plugin)); 
}


JPlugin::~JPlugin() {

    // Call FinalizePlugin()
    typedef void FinalizePlugin_t(JApplication* app);
    FinalizePlugin_t* finalize_proc = (FinalizePlugin_t*) dlsym(m_handle, "FinalizePlugin");
    if (finalize_proc) {
        LOG_INFO(m_logger) << "Finalizing plugin \"" << m_name << "\"" << LOG_END;
        (*finalize_proc)(m_app);
    }

    // Close plugin handle
    dlclose(m_handle);
    LOG_DEBUG(m_logger) << "Unloaded plugin \"" << m_name << "\"" << LOG_END;
}


bool JPluginLoader::is_path(const std::string user_plugin) {
    return user_plugin.find('/') != -1;
}

std::string JPluginLoader::normalize_name(const std::string user_plugin) {
    if (user_plugin.substr(user_plugin.size() - 3) == ".so") {
        return user_plugin.substr(0, user_plugin.size() - 3);
    }
    return user_plugin;
}

std::string JPluginLoader::extract_name_from_path(const std::string user_plugin) {
    // Ideally we would just do this, but we want to be C++17 compatible
    // return std::filesystem::path(filesystem_path).filename().stem().string();
    
    size_t pos_begin = user_plugin.find_last_of('/');
    if (pos_begin == std::string::npos) pos_begin = 0;
    
    size_t pos_end = user_plugin.find_last_of('.');
    //if (pos_end == std::string::npos) pos_end = filesystem_path.size();

    return user_plugin.substr(pos_begin+1, pos_end-pos_begin-1);
}

bool ends_with(const std::string& s, char c) {
    // Ideally we would just do s.ends_with(c), but we want to be C++17 compatible
    if (s.empty()) return false;
    return s.back() == c;
}

std::string JPluginLoader::make_path_from_name(std::string name, const std::string& path_prefix) {
    std::ostringstream oss;
    oss << path_prefix;
    if (!ends_with(path_prefix, '/')) {
        oss << "/";
    }
    oss << name;
    oss << ".so";
    return oss.str();
}

std::string JPluginLoader::find_first_valid_path(const std::string& name, std::ostringstream& debug_log) {

    for (const std::string& path_prefix : m_plugin_paths) {
        auto path = make_path_from_name(name, path_prefix);

        if (validate_path(path)) {
            debug_log << "    " << path << " => Found" << std::endl;
            return path;
        }
        else {
            debug_log << "    " << path << " => Not found" << std::endl;
        }
    }
    return "";
}


bool JPluginLoader::validate_path(const std::string& path) {
    return (access(path.c_str(), F_OK) != -1);
}


