//
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <thread>
#include <cstdio>
#include <chrono>
using namespace std;


#include <JANA/JApplication.h>
#include <JANA/CLI/JMain.h>
#include <JANA/Utils/JCpuInfo.h>
#include <JANA/Services/JParameterManager.h>

#include <pybind11/pybind11.h>
#include <pybind11/embed.h>
namespace py = pybind11;
#include "JEventProcessorPY.h"

static py::module_ *PY_MODULE = nullptr;       // set at end of JANA_MODULE_DEF
static py::module_ PY_MODULE_JSON;  // set at end of JANA_MODULE_DEF
// static py::module_ PY_MODULE_JSON = py::none();  // This will prevent the module from loading!
bool PY_INITIALIZED = false;
static bool PY_MODULE_INSTANTIATED_JAPPLICATION = false;
static JApplication *pyjapp = nullptr;

// Trivial wrappers for JApplication (and friends)
// n.b. The default values here will NEVER be used. They must be passed in explicitly to
// pybind11. They are only here for convenience!
inline void     janapy_Start(void) { PY_INITIALIZED = true; }
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
inline uint32_t janapy_SetNThreads(int Nthreads) { auto prev = pyjapp->GetNThreads(); pyjapp->Scale(Nthreads); return prev; }
inline size_t   janapy_GetNcores(void) { return JCpuInfo::GetNumCpus(); }
inline void     janapy_PrintStatus(void) { pyjapp->PrintStatus(); }
inline void     janapy_PrintParameters(bool all=false) { pyjapp->GetJParameterManager()->PrintParameters(all); }
inline string   janapy_GetParameterValue(string key) { return pyjapp->GetJParameterManager()->Exists(key) ? pyjapp->GetParameterValue<string>(key):"Not Defined"; }

inline void     janapy_Run(void)
{
    if(PY_MODULE_INSTANTIATED_JAPPLICATION) {
        // Being run as python module
        auto options = jana::ParseCommandLineOptions(0, nullptr, false);
        auto exit_code = jana::Execute(pyjapp, options);
        LOG << "JANA processing complete. Exit code: " << exit_code << LOG_END;
    }else{
        // Being run from C++ executable with janapy plugin
        janapy_Start();
    }
}

// Not sure of the clean way to have python cast val object to string before passing to us ...
inline void     janapy_SetParameterValue(string key, py::object valobj)
{
//    const char* val = PyUnicode_AsUTF8( PyObject_Str(valobj) );
    stringstream ss;
    try {
        auto val = valobj.cast<string>();
        ss << val;
    }catch(...){
        try {
            auto val = valobj.cast<double>();
            ss << val;
        }catch(...) {
            try {
                auto val = valobj.cast<long>();
                ss << val;
            }catch(...){}
        }
    }
    pyjapp->SetParameterValue<string>( key, ss.str() );
}

//-----------------------------------------
// janapy_AddProcessor
//-----------------------------------------
inline void janapy_AddProcessor(py::object &pyproc ) {

    // The JEventProcessorPY is instantiated from python by vitue of
    // it being a base class for whatever jana.JEventProcessor class
    // the python script defines. Here though we set some data members
    // only accessible via C++.
    JEventProcessorPY *proc = pyproc.cast<JEventProcessorPY *>();
    proc->pymodule = PY_MODULE;
    proc->pymodule_json = &PY_MODULE_JSON;

    if (pyjapp != nullptr) {
        cout << "[INFO] Adding JEventProcessorPY" << endl;
        pyjapp->Add( new JEventProcessorPYTrampoline(pyjapp, proc) );
    }else {
        cerr << "[ERROR] pyjapp not set before call to janapy_AddProcessor() !!" << endl;
    }
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
py::class_<JEventProcessorPY>(m, "JEventProcessor")\
.def(py::init<py::object&>())\
.def("Init",       &JEventProcessorPY::Init)\
.def("Process",    &JEventProcessorPY::Process)\
.def("Finish",     &JEventProcessorPY::Finish)\
.def("Prefetch",   &JEventProcessorPY::Prefetch, py::arg("fac_name"), py::arg("tag")="")\
.def("Get",        &JEventProcessorPY::Get, py::arg("fac_name"), py::arg("tag")="");\
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
m.def("SetNThreads",                 &janapy_SetNThreads,                 "Set number of JThread objects. (returns previous number)"); \
m.def("GetNcores",                   &janapy_GetNcores,                   "Return number of cores reported by system (full + logical)."); \
m.def("PrintStatus",                 &janapy_PrintStatus,                 "Print the status of the current job to the screen"); \
m.def("PrintParameters",             &janapy_PrintParameters,             "Print config. parameters (give argument True to print all parameters)"); \
m.def("GetParameterValue",           &janapy_GetParameterValue,           "Return value of given configuration parameter."); \
m.def("SetParameterValue",           &janapy_SetParameterValue,           "Set configuration parameter."); \
m.def("AddProcessor",                &janapy_AddProcessor,                "Add an event processor"); \
\
PY_MODULE = &m;\
try{\
    PY_MODULE_JSON = py::module_::import("json");\
}catch(pybind11::type_error &e){\
    _DBG_<<" Error importing python json module!"<<std::endl;\
}catch(...){\
    _DBG_<<" Error importing python json module!"<<std::endl;\
}\
\
//================================================================================

