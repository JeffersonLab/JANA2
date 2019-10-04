//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//


#include "JPluginLoader.h"
#include "JComponentManager.h"
#include "JLoggingService.h"

#include <dlfcn.h>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <sched.h>
#include <set>

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
    for (std::string& n : _plugins_to_include) {
        if (n == plugin_name) {
            return;
        }
    }
    _plugins_to_include.push_back(plugin_name);
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
    for (std::string& n : _plugin_paths) {
        if (n == path) {
            return;
        }
    }
    _plugin_paths.push_back(path);
}


void JPluginLoader::attach_plugins(JComponentManager* jcm) {
    /// Loop over list of plugin names added via AddPlugin() and
    /// actually attach and initialize them. See AddPlugin method
    /// for more.

    // The JANA_PLUGIN_PATH specifies directories to search
    // for plugins that were explicitly added through AddPlugin(...).
    // Multiple directories can be specified using a colon(:) separator.
    const char* jpp = getenv("JANA_PLUGIN_PATH");
    if (jpp) {
        std::stringstream ss(jpp);
        std::string path;
        while (getline(ss, path, ':')) add_plugin_path(path);
    }

    // Default plugin search path
    add_plugin_path(".");
    if (const char* ptr = getenv("JANA_HOME")) add_plugin_path(std::string(ptr) + "/plugins");

    // Add plugins specified via PLUGINS configuration parameter
    // (comma separated list).
    std::set<std::string> exclusions(_plugins_to_exclude.begin(), _plugins_to_exclude.end());

    // Loop over plugins
    std::stringstream err_mess;
    for (std::string plugin : _plugins_to_include) {
        if (exclusions.find(plugin) != exclusions.end()) {
            LOG_IF(_verbose) << "Excluding plugin `" << plugin << "`" << LOG_END;
            continue;
        }
        // Sometimes, the user will include the ".so" suffix in the
        // plugin name. If they don't, then we add it here.
        if (plugin.substr(plugin.size() - 3) != ".so") plugin = plugin + ".so";

        // Loop over paths
        bool found_plugin = false;
        for (std::string path : _plugin_paths) {
            std::string fullpath = path + "/" + plugin;
            LOG_IF(_verbose) << "Looking for '" << fullpath << "' ...." << LOG_END;
            if (access(fullpath.c_str(), F_OK) != -1) {
                LOG_IF(_verbose) << "Found!" << LOG_END;
                try {
                    jcm->next_plugin(plugin);
                    attach_plugin(jcm, fullpath.c_str());
                    found_plugin = true;
                    break;
                } catch (...) {
                    err_mess << "Tried to attach: \"" << fullpath << "\"" << std::endl;
                    err_mess << "  -- error message: " << dlerror() << std::endl;
                    continue;
                }
            }
            LOG_IF(_verbose) << "Failed to attach '" << fullpath << "'" << LOG_END;
        }

        // If we didn't find the plugin, then complain and quit
        if (!found_plugin) {

            LOG_FATAL(_logger) << "\n*** Couldn't find plugin '" << plugin << "'! ***\n" <<
                               "***        Make sure the JANA_PLUGIN_PATH environment variable is set correctly.\n" <<
                               "***        To see paths checked, set JANA:DEBUG_PLUGIN_LOADING=1\n" <<
                               "***        Some hints to the error follow:\n\n" << err_mess.str() << LOG_END;

            exit(-1);
        }
    }
}


void JPluginLoader::attach_plugin(JComponentManager* jcm, std::string soname) {

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
        LOG_DEBUG(_logger) << dlerror() << LOG_END;
        throw "dlopen failed";
    }

    // Look for an InitPlugin symbol
    typedef void InitPlugin_t(JApplication* app);
    //typedef void InitPlugin_t(JComponentManager* jcm, JServiceLocator* sl);
    // TODO: Convert InitPlugin sig to (builder, servicelocator) -> void
    InitPlugin_t* plugin = (InitPlugin_t*) dlsym(handle, "InitPlugin");
    if (plugin) {
        LOG_INFO(_logger) << "Initializing plugin \"" << soname << "\"" << LOG_END;
        (*plugin)(_app);
        _sohandles.push_back(handle);
    } else {
        dlclose(handle);
        LOG_IF(_verbose) << "Plugin \"" << soname << "\" does not have an InitPlugin() function. Ignoring."
                                     << LOG_END;
    }
}


JPluginLoader::JPluginLoader(JApplication* app, JParameterManager* params) {

    _app = app;

    if (params != nullptr) {

        params->SetDefaultParameter("plugins", _plugins_to_include, "");
        params->SetDefaultParameter("plugins_to_ignore", _plugins_to_exclude, "");

        params->SetDefaultParameter("JANA:DEBUG_PLUGIN_LOADING", _verbose,
                                    "Trace the plugin search path and display any loading errors");
    }
}



