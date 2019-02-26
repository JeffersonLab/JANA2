//
//    File: janapy/janapy.cc
// Created: Fri Dec 21 23:05:11 EST 2018
// Creator: davidl (on Darwin amelia.jlab.org 17.7.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
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
#include <Python.h>

#include <JANA/JApplication.h>
#include <JANA/JThreadManager.h>
#include <JANA/JCpuInfo.h>

void JANA_PythonModuleInit(JApplication *sApp);

static bool PY_INITIALIZED = false; // See JANA_PythonModuleInit

extern "C"{
void InitPlugin(JApplication *app){
	InitJANAPlugin(app);

	// Launch thread to handle Python interpreter.
	std::thread thr(JANA_PythonModuleInit, app);
	thr.detach();

	// Wait for interpreter to initialize and possibly run script.
	// This allows more control from python by stalling the initialization
	// so it has a chance to modify things a bit before initialization
	// completes and data processing starts.
	while( !PY_INITIALIZED ) std::this_thread::sleep_for (std::chrono::milliseconds(100));
}
} // "C"

//..........................................................
// The following effectively make a template out of "PV" so it can
// be used to convert all types to PyObjects
template<typename T> PyObject* PV(T v){ return Py_BuildValue("i", (int)v); }
template<> PyObject* PV<int>(int v){ return Py_BuildValue("i", v); }
template<> PyObject* PV<long>(long v){ return Py_BuildValue("l", v); }
template<> PyObject* PV<float>(float v){ return Py_BuildValue("f", v); }
template<> PyObject* PV<double>(double v){ return Py_BuildValue("d", v); }
template<> PyObject* PV<string>(string v){ return Py_BuildValue("s", v.c_str()); }
template<> PyObject* PV<const char*>(const char* v){ return Py_BuildValue("s", v); }

// 1-D container types (vector, list, set, ...)
template<typename T>
PyObject* PVlist( T& c ){
	PyObject *PList = PyList_New( c.size() );
	std::size_t i=0;
	for( auto v : c ) PyList_SET_ITEM( PList, i++, PV(v));
	return PList;
}

// map containers
template<typename K, typename V>
PyObject* PVdict( std::map<K,V> &m ){
	PyObject *PDict = PyDict_New();
	for( auto p : m ) PyDict_SetItem( PDict, PV(p.first), PV(p.second));
	return PDict;
}
//..........................................................

//-------------------------------------
// janapy_Start
//-------------------------------------
static PyObject* janapy_Start(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":Start")) return NULL;
	PY_INITIALIZED = true;
	return PV("");
}

//-------------------------------------
// janapy_Quit
//-------------------------------------
static PyObject* janapy_Quit(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":Quit")) return NULL;
	japp->Quit();
	return PV("");
}

//-------------------------------------
// janapy_Stop
//-------------------------------------
static PyObject* janapy_Stop(PyObject *self, PyObject *args)
{
	int wait_until_idle=false;
	if(!PyArg_ParseTuple(args, "|i:Stop", &wait_until_idle)) return NULL;
	japp->Stop(wait_until_idle);
	return PV("");
}

//-------------------------------------
// janapy_Resume
//-------------------------------------
static PyObject* janapy_Resume(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":Resume")) return NULL;
	japp->Resume();
	return PV("");
}

//-------------------------------------
// janapy_WaitUntilAllThreadsRunning
//-------------------------------------
static PyObject* janapy_WaitUntilAllThreadsRunning(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":WaitUntilAllThreadsRunning")) return NULL;
	PY_INITIALIZED = true; // (in case user doesn't call Start before calling this)
	japp->GetJThreadManager()->WaitUntilAllThreadsRunning();
	return PV("");
}

//-------------------------------------
// janapy_WaitUntilAllThreadsIdle
//-------------------------------------
static PyObject* janapy_WaitUntilAllThreadsIdle(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":WaitUntilAllThreadsIdle")) return NULL;
	PY_INITIALIZED = true; // (in case user doesn't call Start before calling this)
	japp->GetJThreadManager()->WaitUntilAllThreadsIdle();
	return PV("");
}

//-------------------------------------
// janapy_WaitUntilAllThreadsEnded
//-------------------------------------
static PyObject* janapy_WaitUntilAllThreadsEnded(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":WaitUntilAllThreadsEnded")) return NULL;
	PY_INITIALIZED = true; // (in case user doesn't call Start before calling this)
	japp->GetJThreadManager()->WaitUntilAllThreadsEnded();
	return PV("");
}

//-------------------------------------
// janapy_GetNtasksCompleted
//-------------------------------------
static PyObject* janapy_GetNtasksCompleted(PyObject *self, PyObject *args)
{
	char name[512] = "";
	if(!PyArg_ParseTuple(args, "|s:GetNtasksCompleted", name)) return NULL;
	return PV( japp->GetNtasksCompleted( name ) );
}

//-------------------------------------
// janapy_GetNeventsProcessed
//-------------------------------------
static PyObject* janapy_GetNeventsProcessed(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":GetNeventsProcessed")) return NULL;
	return PV( japp->GetNeventsProcessed() );
}

//-------------------------------------
// janapy_GetIntegratedRate
//-------------------------------------
static PyObject* janapy_GetIntegratedRate(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":GetIntegratedRate")) return NULL;
	return PV( japp->GetIntegratedRate() );
}

//-------------------------------------
// janapy_GetIntegratedRates
//-------------------------------------
static PyObject* janapy_GetIntegratedRates(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":GetIntegratedRates")) return NULL;
	std::map<string,double> rates_by_thread;
	japp->GetIntegratedRates(rates_by_thread);
	return PVdict( rates_by_thread );
}

//-------------------------------------
// janapy_GetInstantaneousRate
//-------------------------------------
static PyObject* janapy_GetInstantaneousRate(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":GetInstantaneousRate")) return NULL;
	return PV( japp->GetInstantaneousRate() );
}

//-------------------------------------
// janapy_GetInstantaneousRates
//-------------------------------------
static PyObject* janapy_GetInstantaneousRates(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":GetInstantaneousRates")) return NULL;
	vector<double> rates_by_queue;
	japp->GetInstantaneousRates(rates_by_queue);
	return PVlist( rates_by_queue );
}

//-------------------------------------
// janapy_GetNJThreads
//-------------------------------------
static PyObject* janapy_GetNJThreads(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":GetNJThreads")) return NULL;
	return PV( japp->GetJThreadManager()->GetNJThreads() );
}

//-------------------------------------
// janapy_GetNcores
//-------------------------------------
static PyObject* janapy_GetNcores(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":GetNcores")) return NULL;
	return PV(JCpuInfo::GetNumCpus());
}

//-------------------------------------
// janapy_GetParameterValue
//-------------------------------------
static PyObject* janapy_GetParameterValue(PyObject *self, PyObject *args)
{
	char *str;
	if(!PyArg_ParseTuple(args, "s:GetParameterValue", &str)) return NULL;
	try{
		std::cout << "\n\rLooking for parameter: " << str << "\n\r";
		return PV( japp->GetParameterValue<string>( str ) );
	}catch(...){
		return PV("Not Defined");
	}
}

//-------------------------------------
// janapy_SetParameterValue
//-------------------------------------
static PyObject* janapy_SetParameterValue(PyObject *self, PyObject *args)
{
	char *key;
	char *val;
	if(!PyArg_ParseTuple(args, "ss:SetParameterValue", &key, &val)) return NULL;
	japp->SetParameterValue<string>( key, val );
	return PV( "" );
}

//-------------------------------------
// janapy_SetTicker
//-------------------------------------
static PyObject* janapy_SetTicker(PyObject *self, PyObject *args)
{
	int ticker_on = true;
	if(!PyArg_ParseTuple(args, "|i:SetTicker", &ticker_on)) return NULL;
	japp->SetTicker( ticker_on );
	return PV( "" );
}

//-------------------------------------
// janapy_SetNJThreads
//-------------------------------------
static PyObject* janapy_SetNJThreads(PyObject *self, PyObject *args)
{
	int nthreads=1;
	if(!PyArg_ParseTuple(args, "i:SetNJThreads", &nthreads)) return NULL;
	japp->GetJThreadManager()->SetNJThreads( nthreads );
	return PV( japp->GetJThreadManager()->GetNJThreads() );
}

//-------------------------------------
// janapy_IsQuitting
//-------------------------------------
static PyObject* janapy_IsQuitting(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":IsQuitting")) return NULL;
	return PV( japp->IsQuitting() );
}

//-------------------------------------
// janapy_IsDrainingQueues
//-------------------------------------
static PyObject* janapy_IsDrainingQueues(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":IsDrainingQueues")) return NULL;
	return PV( japp->IsDrainingQueues() );
}

//-------------------------------------
// janapy_PrintStatus
//-------------------------------------
static PyObject* janapy_PrintStatus(PyObject *self, PyObject *args)
{
	if(!PyArg_ParseTuple(args, ":PrintStatus")) return NULL;
	japp->PrintStatus();
	return PV( "" );
}

//-------------------------------------
// janapy_PrintParameters
//-------------------------------------
static PyObject* janapy_PrintParameters(PyObject *self, PyObject *args)
{
	int print_all = 0;
	if(!PyArg_ParseTuple(args, "|i:PrintParameters", &print_all)) return NULL;
	japp->GetJParameterManager()->PrintParameters( print_all );
	return PV( "" );
}


//.....................................................
// Define python methods
static PyMethodDef JANAPYMethods[] = {
	{"Start",                       janapy_Start,                       METH_VARARGS, "Allow JANA system to start processing data. (Not needed for short scripts.)"},
	{"Quit",                        janapy_Quit,                        METH_VARARGS, "Tell JANA to quit gracefully"},
	{"Stop",                        janapy_Stop,                        METH_VARARGS, "Tell JANA to (temporarily) stop event processing. If optional agrument is True then block until all threads are stopped."},
	{"Resume",                      janapy_Resume,                      METH_VARARGS, "Tell JANA to resume event processing."},
	{"WaitUntilAllThreadsRunning",  janapy_WaitUntilAllThreadsRunning,  METH_VARARGS, "Wait until all threads have entered the running state."},
	{"WaitUntilAllThreadsIdle",     janapy_WaitUntilAllThreadsIdle,     METH_VARARGS, "Wait until all threads have entered the idle state."},
	{"WaitUntilAllThreadsEnded",    janapy_WaitUntilAllThreadsEnded,    METH_VARARGS, "Wait until all threads have entered the ended state."},
	{"GetNtasksCompleted",          janapy_GetNtasksCompleted,          METH_VARARGS, "Return the number of tasks completed. If specified, only count tasks for that JQueue."},
	{"GetNeventsProcessed",         janapy_GetNeventsProcessed,         METH_VARARGS, "Return the number of events processed so far."},
	{"GetIntegratedRate",           janapy_GetIntegratedRate,           METH_VARARGS, "Return integrated rate."},
	{"GetIntegratedRates",          janapy_GetIntegratedRates,          METH_VARARGS, "Return integrated rates for each thread."},
	{"GetInstantaneousRate",        janapy_GetInstantaneousRate,        METH_VARARGS, "Return instantaneous rate."},
	{"GetInstantaneousRates",       janapy_GetInstantaneousRates,       METH_VARARGS, "Return instantaneous rates for each thread."},
	{"GetNJThreads",                janapy_GetNJThreads,                METH_VARARGS, "Return current number of JThread objects."},
	{"GetNcores",                   janapy_GetNcores,                   METH_VARARGS, "Return number of cores reported by system (full + logical)."},
	{"GetParameterValue",           janapy_GetParameterValue,           METH_VARARGS, "Return value of given configuration parameter."},
	{"SetParameterValue",           janapy_SetParameterValue,           METH_VARARGS, "Set configuration parameter."},
	{"SetTicker",                   janapy_SetTicker,                   METH_VARARGS, "Set ticker on/off that updates at bottom of screen."},
	{"SetNJThreads",                janapy_SetNJThreads,                METH_VARARGS, "Set the number of JThread objects by creating or deleting."},
	{"IsQuitting",                  janapy_IsQuitting,                  METH_VARARGS, "Returns true if the application quit flag has been set."},
	{"IsDrainingQueues",            janapy_IsDrainingQueues,            METH_VARARGS, "Returns true if the application draining queues flag is set indicating all events have been read in."},
	{"PrintStatus",                 janapy_PrintStatus,                 METH_VARARGS, "Print current JANA status."},
	{"PrintParameters",             janapy_PrintParameters,             METH_VARARGS, "Print configuration parameters. Pass True to print all. Otherwise, only non-default ones will be printed."},
	{NULL, NULL, 0, NULL}
};
//.....................................................

//-------------------------------------
// JANA_PythonModuleInit
//-------------------------------------
void JANA_PythonModuleInit(JApplication *sApp)
{
	/// This will initialize the embedded Python interpreter and import the
	/// JANA module (defined by the routines in this file). This gets called
	/// when the janapy plugin gets initialized. It will then automatically
	/// look for a file whose name is given by either the JANA_PYTHON_FILE
	/// config. parameter or environment variable in that order. If neither
	/// exists, it looks for a file name "jana.py" in the current working
	/// directory and if found, executes that. If you do not wish to execute
	/// a python file, then you can set the JANA_PYTHON_FILE value to an empty
	/// string (or better yet, just not attach the janapy plugin!).
	///
	/// Note that janapy creates a dedicated thread that the python interpreter
	/// runs in.
	///
	/// IMPORTANT: The janapy InitPlugin routine will block (resulting in the whole
	/// JANA system pausing) until either this routine finishes or the python
	/// script it invokes calls "jana.Run()". This is to give the python script
	/// a chance to modify running conditions prior to event processing starting.
	/// If the python script intends to run throughout the life of the process,
	/// then it MUST call jana.Run() at some point. If the script is small and only
	/// runs for a short time, then you don't need to call it since it will be
	/// called automatically when the script ends.

	// Check if interpreter is already initialized and do nothing at all if it is.
	if( Py_IsInitialized() ){
		jout << "Python already initialized! Skipping initialization of janapy!" << std::endl;
		PY_INITIALIZED = true;
		return;
	}

	// Initialize interpreter and register the jana module
	jout << "Initializing embedded Python ... " << std::endl;
	PyEval_InitThreads();
	Py_Initialize();
	Py_InitModule("jana", JANAPYMethods);

	// Get name of python file to execute
	string fname = "jana.py";
	try{
		fname = sApp->GetParameterValue<string>("JANA_PYTHON_FILE");
	}catch(...){
		auto JANA_PYTHON_FILE = getenv("JANA_PYTHON_FILE");
		if( JANA_PYTHON_FILE ) fname = JANA_PYTHON_FILE;
	}
	
//	PyGILState_STATE gstate = PyGILState_Ensure();
//	PyEval_ReleaseLock();
//	PyThreadState *_save = PyEval_SaveThread();
//	Py_BEGIN_ALLOW_THREADS
	
	auto fil = std::fopen(fname.c_str(), "r");
	if( fil ) {
		jout << "Executing Python script: " << fname << " ..." << std::endl;
		const char *argv = fname.c_str();
		PySys_SetArgv( 1, (char**)&argv );
		PyRun_AnyFileEx( fil, NULL, 1 );
	}else if( fname != "jana.py" ){
		jerr << std::endl << "Unable to open \"" << fname << "\"! Quitting." << std::endl << std::endl;
		sApp->Quit();
		PY_INITIALIZED = true;
		std::this_thread::sleep_for(std::chrono::seconds(1));
		exit(-1);
	}
	
//	Py_END_ALLOW_THREADS
	
	PY_INITIALIZED = true;
}
