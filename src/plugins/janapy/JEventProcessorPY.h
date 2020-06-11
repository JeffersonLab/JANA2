
#ifndef _JEVEVENTPROCESSOR_PY_H_
#define _JEVEVENTPROCESSOR_PY_H_

#include <mutex>
#include <iostream>
using std::cout;
using std::endl;

#include "pybind11/pybind11.h"
namespace py = pybind11;


#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>

class JEventProcessorPY : public JEventProcessor{

    public:

    JEventProcessorPY(py::object &py_obj):pyobj(py_obj){

        cout << "JEventProcessorPY constructor called with py:object : " << this  << endl;

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

    py::object &pyobj; // _self_
    py::object pymInit;
    py::object pymProcess;
    py::object pymFinish;
    bool has_pymInit    = false;
    bool has_pymProcess = false;
    bool has_pymFinish  = false;

    mutex pymutex;

};

#endif  // _JEVEVENTPROCESSOR_PY_H_
