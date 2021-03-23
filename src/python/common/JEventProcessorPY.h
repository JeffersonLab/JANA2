
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

#ifdef HAVE_ROOT
#include <TObject.h>
#include <TClass.h>
#include <TDataMember.h>
#include <TMethodCall.h>
#include <Tlist.h>
#endif // HAVE_ROOT

#include <JANA/JEventProcessor.h>
#include <JANA/JEvent.h>
#include <JANA/Utils/JStringification.h>

std::mutex pymutex; // This is bad practice to put this in a header, but it is needed in both the plugin and the module
extern bool PY_INITIALIZED;  // declared in janapy.h

//#pragma GCC visibility push(hidden)

class JEventProcessorPY {

    public:

    //----------------------------------------
    // JEventProcessorPY
    JEventProcessorPY(py::object &py_obj):pyobj(py_obj),jstringification(new JStringification){

        cout << "JEventProcessorPY constructor called with py:object : " << this  << endl;

        // Get the name of the Python class inheriting from JEventProcessorPY
        // so it can be displayed as the JEventProcessor name (see JEventProcessorPYTrampoline)
        auto name_obj = py_obj.get_type().attr("__name__");
        class_name = py::cast<std::string>(name_obj);

        try { pymInit    = pyobj.attr("Init"   );  has_pymInit    = true; }catch(...){}
        try { pymProcess = pyobj.attr("Process");  has_pymProcess = true; }catch(...){}
        try { pymFinish  = pyobj.attr("Finish" );  has_pymFinish  = true; }catch(...){}

    }

    //----------------------------------------
    // ~JEventProcessorPY
    ~JEventProcessorPY() {
        cout << "JEventProcessorPY destructor called : " << this  << endl;
    }

    //----------------------------------------
    // SetJApplication
    //
    /// This sets the internal mApplication member. It is called by the
    /// JEventProcessorPYTrampoline class constructor, which itself is
    /// only called when the Python jana.AddProcessor(proc) method is
    /// called.
    void SetJApplication(JApplication *japp) {
        mApplication = japp;
    }

    //----------------------------------------
    // Init
    void Init(void){

        if( has_pymInit && PY_INITIALIZED ) {
            lock_guard<mutex> lck(pymutex);
            pymInit();
        }

    }

    //----------------------------------------
    // Process
    void Process(const std::shared_ptr<const JEvent>& aEvent){

        // Prefetch any factory in the prefetch list.
        // This does not actually fetch them, but does activate the factories
        // to ensure the objects are already created before locking the mutex.
        // We try fetching as both JObject and TObject for the rare case when
        // the objects may be TObjects but not JObjects. We cannot currently
        // handle the case where the factory object is neither a JObject nor
        // a TObject.
        for( auto p : prefetch_factories ){

            auto fac = aEvent->GetFactory( p.first, p.second);
            if( fac == nullptr ){
                jerr << "Unable to find factory specified for prefetching: factory=" << p.first << " tag=" << p.second << std::endl;
            }else {
                auto v = fac->GetAs<JObject>();
#ifdef HAVE_ROOT
                if( v.empty() )fac->GetAs<TObject>();
#endif // HAVE_ROOT

                _DBG_<<"Prefetching from factory: " << p.first <<":" << p.second << "  - " << v.size() << " objects" <<std::endl;
            }
        }

        if( has_pymProcess  && PY_INITIALIZED ) {

            // According to the Python documentation we should be wrapping the call to pmProcess() below
            // in the following that activates the GIL lock. In practice, this seemed to allow each thread
            // to call pymProcess(), once, but then the program stalled. Hence, we use our own mutex.
            // PyGILState_STATE gstate = PyGILState_Ensure();
            // PyGILState_Release(gstate);
            lock_guard<mutex> lck(pymutex);

            // This magic line creates a shared_ptr on the stack with a custom deleter.
            // The custom deleter is used to reset the mEvent data member back to
            // nullptr so that it does not hold on the the JEvent after we return
            // from this method. We use this trick to ensure it happens even if an
            // exception is thrown from pymProcess().
            std::shared_ptr<int> mEvent_free(nullptr, [=](int *ptr){ mEvent = nullptr;});
            mEvent = aEvent; // remember this JEvent so it can be used in calls to Get()

            pymProcess();
        }
    }

    //----------------------------------------
    // Finish
    void Finish(void){

        if( has_pymFinish  && PY_INITIALIZED ) {
            lock_guard<mutex> lck(pymutex);
            pymFinish();
        }

    }

    //----------------------------------------
    // Prefetch
    void Prefetch(py::object &fac_name, py::object tag = py::none()){

        /// This is called from python to register factories to be activated
        /// before the Process method of the python JEventProcessor class
        /// is called. Since the python code must be executed in serial, this
        /// allows object generation by factories to be done in parallel before
        /// that to try and minimize the time in serial operations.
        ///
        /// When calling from python, the first argument may be any of a
        /// string, list, or dictionary. The second argument is for the optional
        /// factory tag which is only considered if the first argument is a
        /// string.
        ///
        /// If the first argument is a string, it is taken as the data type to
        /// prefetch.
        ///
        /// If the first argument is a list, it is taken to be the names of multiple
        /// data types to prefetch. For this case, the default (i.e. empty) factory
        /// tag is used for all of them.
        ///
        /// If the first argument is a dictionary, then the keys are taken as
        /// the data types and corresponding values taken as the factory tags.

        if( py::isinstance<py::dict>(fac_name) ){
            // Python dictionary was passed
            for(auto p : fac_name.cast<py::dict>()){
                auto &fac_obj = p.first;
                auto &tag_obj = p.second;
                std::string fac_name_str = py::str(fac_obj);
                std::string tag_str = tag_obj.is(py::none()) ? "":py::str(tag_obj);
                prefetch_factories[fac_name_str] = tag_str;
            }
        }else if( py::isinstance<py::list>(fac_name) ){
            // Python list was passed
            for(auto &fac_obj : fac_name.cast<py::list>()){
                std::string fac_name_str = py::str(fac_obj);
                prefetch_factories[fac_name_str] = "";
            }
        }else if( py::isinstance<py::str>(fac_name) ){
            // Python string was passed
            std::string fac_name_str = py::str(fac_name);
            std::string tag_str = tag.is(py::none()) ? "":py::str(tag);
            prefetch_factories[fac_name_str] = tag_str;
        }else{
            jerr << "Unknown type passed to Prefetch: " << std::string(py::str(fac_name)) << std::endl;
        }
    }

    //----------------------------------------
    // Get
    py::object Get(py::object &fac_name, py::object tag = py::none()) {
        std::string fac_name_str = py::str(fac_name);
        std::string tag_str = tag.is(py::none()) ? "":py::str(tag);

        std::vector<std::string> json_vec;
        jstringification->GetObjectSummariesAsJSON(json_vec, mEvent, fac_name_str, tag_str);

        py::list list;
        for(auto obj_json : json_vec){

            try {
                auto json_loads = pymodule_json->attr("loads" );
                auto dict = json_loads( py::str(obj_json) );
                list.append( dict );
            }catch(...){
                jerr << "Python json loads function not available!" << std::endl;
            }
        }

        return list;
    }

    // Data members
    py::module_ *pymodule = nullptr;       // This gets set in janapy_AddProcessor
    py::module_ *pymodule_json = nullptr;  // This gets set in janapy_AddProcessor
    std::string class_name = "JEventProcssorPY";
    py::object &pyobj; // _self_
    py::object pymInit;
    py::object pymProcess;
    py::object pymFinish;
    bool has_pymInit    = false;
    bool has_pymProcess = false;
    bool has_pymFinish  = false;

    JApplication *mApplication = nullptr;
    std::shared_ptr<const JEvent> mEvent;
    std::map<std::string, std::string> prefetch_factories;
    std::shared_ptr<const JStringification> jstringification;
};

class JEventProcessorPYTrampoline: public JEventProcessor {

public:
    JEventProcessorPYTrampoline(JApplication *japp, JEventProcessorPY *jevent_proc):JEventProcessor(japp),jevent_proc_py(jevent_proc){
        SetTypeName(jevent_proc->class_name);
        jevent_proc_py->SetJApplication(japp); // copy JApplication pointer to our JEventProcessorPY
    }

    void Init(void){ jevent_proc_py->Init(); }
    void Process(const std::shared_ptr<const JEvent>& aEvent){ jevent_proc_py->Process(aEvent); }
    void Finish(void){ jevent_proc_py->Finish(); }

private:
    JEventProcessorPY *jevent_proc_py = nullptr;
};
#endif  // _JEVEVENTPROCESSOR_PY_H_
