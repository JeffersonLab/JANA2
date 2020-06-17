//
//    File: janapy.h
// Created: Tue Jun 16 14:28:11 EST 2019
// Creator: davidl (on Darwin amelia.jlab.org 17.7.0 i386)
//
// ------ Last repository commit info -----
// [ Date: Tue Feb 26 18:13:39 2019 -0500 ]
// [ Author: Nathan Brei <nbrei@halld3.jlab.org> ]
// [ Source: src/plugins/janapy/janapy.cc ]
// [ Revision: c15aad0b0dec2e6f8f29d1f727b2daec6c7cf376 ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
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

#include <thread>
#include <cstdio>
#include <chrono>
using namespace std;


#include <JANA/JApplication.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Services/JParameterManager.h>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
namespace py = pybind11;
#include "JEventProcessorPY.h"

static bool PY_INITIALIZED = false;
static JApplication *pyjapp = nullptr;

// Trivial wrappers for JApplication (and friends)
// n.b. The default values here will NEVER be used. They must be passed in explicitly to
// pybind11. They are only here for convenience!
inline void     janapy_Start(void) { PY_INITIALIZED = true; }
inline void     janapy_Run(void) { pyjapp->Run(); }
inline void     janapy_Quit(bool skip_join=false) { pyjapp->Quit(skip_join); }
inline void     janapy_Stop(bool wait_until_idle=false) { pyjapp->Stop(wait_until_idle); }
inline void     janapy_Resume(void) { pyjapp->Resume(); }

inline bool     janapy_IsInitialized(void) { return pyjapp->IsInitialized(); }
inline bool     janapy_IsQuitting(void) { return pyjapp->IsQuitting(); }
inline bool     janapy_IsDrainingQueues(void) { return pyjapp->IsDrainingQueues(); }

inline void     janapy_AddPlugin(string plugin_name) { pyjapp->AddPlugin(plugin_name); }
inline void     janapy_AddPluginPath(string path) { pyjapp->AddPluginPath(path); }
inline void     janapy_AddEventSource(string source) { pyjapp->Add( source ); }
inline void     janapy_SetTicker(bool ticker_on=true) { pyjapp->SetTicker(ticker_on); }
inline uint64_t janapy_GetNeventsProcessed(void) { return pyjapp->GetNEventsProcessed(); }
inline float    janapy_GetIntegratedRate(void) { return pyjapp->GetIntegratedRate(); }
inline float    janapy_GetInstantaneousRate(void) { return pyjapp->GetInstantaneousRate(); }
inline uint32_t janapy_GetNThreads(void) { return pyjapp->GetNThreads(); }
inline size_t   janapy_GetNcores(void) { return JCpuInfo::GetNumCpus(); }
inline string   janapy_GetParameterValue(string key) { return pyjapp->GetJParameterManager()->Exists(key) ? pyjapp->GetParameterValue<string>(key):"Not Defined"; }
inline void     janapy_SetParameterValue(string key, string val) { pyjapp->SetParameterValue<string>( key, val ); }


inline void janapy_AddProcessor(py::object &pyproc )
{
    cout << "JANAPY2_AddProcessor called!" << endl;
    JEventProcessorPY *proc = pyproc.cast<JEventProcessorPY *>();
    pyjapp->Add( proc );
}

//================================================================================
// Module definition
//
// The following macro is used to define the jana module in both the plugin
// (embedded interpreter) and python module forms. These are built using
// pybind11 macros PYBIND11_EMBEDDED_MODULE and PYBIND11_MODULE respectively.
// Ideally, we could combine the JANA plugin and python module into a single
// binary and wouldn't need to bother with the ugliness of using a macro like
// this. Alas, that does not seem to be (easily) possible. Thus the macro is
// defined here and used in both modules/jana/jana_module.cc and
// plugins/janapy/janapy_plugin.cc

#define JANA_MODULE_DEF \
\
/* JEventProcessor */ \
py::class_<JEventProcessorPY>(m, "JEventProcessor") \
.def(py::init<py::object&>()) \
.def("Init", &JEventProcessorPY::Init) \
.def("Process", &JEventProcessorPY::Process); \
\
/* C-wrapper routines */ \
m.def("Start",                       &janapy_Start,                       "Allow JANA system to start processing data. (Not needed for short scripts.)"); \
m.def("Run",                         &janapy_Run,                         "Begin processing events (use when running python as an extension)"); \
m.def("Quit",                        &janapy_Quit,                        "Tell JANA to quit gracefully", py::arg("skip_join")=false); \
m.def("Stop",                        &janapy_Stop,                        "Tell JANA to (temporarily) stop event processing. If optional agrument is True then block until all threads are stopped."); \
m.def("Resume",                      &janapy_Resume,                      "Tell JANA to resume event processing."); \
\
m.def("IsInitialized",               &janapy_IsInitialized,               "Check if JApplication has already been initialized."); \
m.def("IsQuitting",                  &janapy_IsQuitting,                  "Check if JApplication is in the process of quitting."); \
m.def("IsDrainingQueues",            &janapy_IsDrainingQueues,            "Check if JApplication is in the process of draining the queues."); \
\
m.def("AddPlugin",                   &janapy_AddPlugin,                   "Add a plugin to the list of plugins to be attached (call before calling Run)"); \
m.def("AddPluginPath",               &janapy_AddPluginPath,               "Add directory to plugin search path"); \
m.def("AddEventSource",              &janapy_AddEventSource,              "Add an event source (e.g. filename). Can be given multiple arguments and/or called multiple times."); \
m.def("SetTicker",                   &janapy_SetTicker,                   "Turn off/on the standard screen ticker", py::arg("ticker_on")=true); \
m.def("GetNeventsProcessed",         &janapy_GetNeventsProcessed,         "Return the number of events processed so far."); \
m.def("GetIntegratedRate",           &janapy_GetIntegratedRate,           "Return integrated rate."); \
m.def("GetInstantaneousRate",        &janapy_GetInstantaneousRate,        "Return instantaneous rate."); \
m.def("GetNThreads",                 &janapy_GetNThreads,                 "Return current number of JThread objects."); \
m.def("GetNcores",                   &janapy_GetNcores,                   "Return number of cores reported by system (full + logical)."); \
m.def("GetParameterValue",           &janapy_GetParameterValue,           "Return value of given configuration parameter."); \
m.def("SetParameterValue",           &janapy_SetParameterValue,           "Set configuration parameter."); \
m.def("AddProcessor",                &janapy_AddProcessor,                "Add an event processor"); \

//================================================================================

