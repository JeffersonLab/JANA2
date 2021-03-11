
#ifndef _JEVEVENTPROCESSOR_PY_H_
#define _JEVEVENTPROCESSOR_PY_H_

// This defines the JEventProcessorPY classes used to implement
// a JEventProcessor class in Python. The design uses a pair of
// classes with a HasA relationship.
//
// The JEventProcessorPYTrampoline class is a C++ class that
// inherits from JEventProcessor and is what JANA uses for callbacks.
//
// The JEventProcessorPY class is a python class that inherits
// from pyobject in pybind11. It also serves as a base class
// for Python JEventProcessor classes.
//
// Two classes are needed because the design of JANA requires
// ownership of the JEventProcessor object be given to JApplication.
// At the same time, pybind11 insists on ownership of all pyobjects.
//
// n.b. There may actually be a way to do this with one class by
// manipulating the ref counter in the python object


#include <mutex>
#include <iostream>
using std::cout;
using std::endl;

#include <pybind11/pybind11.h>
namespace py = pybind11;


#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

//#pragma GCC visibility push(hidden)

class JEventProcessorPY {

    public:

    JEventProcessorPY(py::object &py_obj):pyobj(py_obj){

        cout << "JEventProcessorPY constructor called with py:object : " << this  << endl;

        // Get the name of the Python class inheriting from JEventProcessorPY
        // so it can be displayed as the JEventProcessor name (see JEventProcessorPYTrampoline)
        auto name_obj = py_obj.get_type().attr("__name__");
        class_name = py::cast<std::string>(name_obj);

        try { pymInit    = pyobj.attr("Init"   );  has_pymInit    = true; }catch(...){}
        try { pymProcess = pyobj.attr("Process");  has_pymProcess = true; }catch(...){}
        try { pymFinish  = pyobj.attr("Finish" );  has_pymFinish  = true; }catch(...){}

    }
    ~JEventProcessorPY() {
        cout << "JEventProcessorPY destructor called : " << this  << endl;
    }

    void Init(void){

        cout << "JEventProcessorPY::Init called " << endl;
        if( has_pymInit ) {
            lock_guard<mutex> lck(pymutex);
            pymInit();
        }

    }

    void Process(const std::shared_ptr<const JEvent>& aEvent){

        cout << "JEventProcessorPY::Process called " << endl;

        // According to the Python documentation we should be wrapping the call to pmProcess() below
        // in the following that activate the GIL lock. In practice, this seemed to allow each thread
        // to call pymProcess(), once, but then the program stalled. Hence, we use our own mutex.
        // PyGILState_STATE gstate = PyGILState_Ensure();
        // PyGILState_Release(gstate);
        if( has_pymProcess ) {
            lock_guard<mutex> lck(pymutex);
            pymProcess();
        }
    }

    void Finish(void){

        cout << "JEventProcessorPY::Finish called " << endl;

        if( has_pymFinish ) {
            lock_guard<mutex> lck(pymutex);
            pymFinish();
        }

    }

    std::string class_name = "JEventProcssorPY";
    py::object &pyobj; // _self_
    py::object pymInit;
    py::object pymProcess;
    py::object pymFinish;
    bool has_pymInit    = false;
    bool has_pymProcess = false;
    bool has_pymFinish  = false;

    mutex pymutex;

};

class JEventProcessorPYTrampoline: public JEventProcessor {

public:
    JEventProcessorPYTrampoline(JEventProcessorPY *jevent_proc):jevent_proc_py(jevent_proc){
        SetTypeName(jevent_proc->class_name);
    }

    void Init(void){ jevent_proc_py->Init(); }
    void Process(const std::shared_ptr<const JEvent>& aEvent){ jevent_proc_py->Process(aEvent); }
    void Finish(void){ jevent_proc_py->Finish(); }

private:
    JEventProcessorPY *jevent_proc_py = nullptr;
};
#endif  // _JEVEVENTPROCESSOR_PY_H_
