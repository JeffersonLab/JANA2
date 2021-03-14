
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
#include <janacontrol/src/janaJSON.h>
#include <janacontrol/src/JControlEventProcessor.h>

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

    //----------------------------------------
    // Init
    void Init(void){

        cout << "JEventProcessorPY::Init called " << endl;
        if( has_pymInit ) {
            lock_guard<mutex> lck(pymutex);
            pymInit();
        }

    }

    //----------------------------------------
    // Process
    void Process(const std::shared_ptr<const JEvent>& aEvent){

        cout << "JEventProcessorPY::Process called " << endl;

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
            }
        }

        if( has_pymProcess ) {

            // According to the Python documentation we should be wrapping the call to pmProcess() below
            // in the following that activates the GIL lock. In practice, this seemed to allow each thread
            // to call pymProcess(), once, but then the program stalled. Hence, we use our own mutex.
            // PyGILState_STATE gstate = PyGILState_Ensure();
            // PyGILState_Release(gstate);
            lock_guard<mutex> lck(pymutex);

            // This magic line creates a shared_ptr on the stack with a custom deleter.
            // The custom deleter is used to reset the _aEvent data member back to
            // nullptr so that it does not hold on the the JEvent after we return
            // from this method. We use this trick to ensure it happens even if an
            // exception is thrown from pymProcess().
            std::shared_ptr<int> _avent_free(nullptr, [=](int *ptr){_aEvent = nullptr;});
            _aEvent = aEvent; // remember this JEvent so it can be used in calls to Get()

            pymProcess();
        }
    }

    //----------------------------------------
    // Finish
    void Finish(void){

        cout << "JEventProcessorPY::Finish called " << endl;

        if( has_pymFinish ) {
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

        std::map<std::string, JObjectSummary> objects;
        GetObjectSummaries(fac_name_str, tag_str, objects);

        py::list list;
        for(auto p : objects){

            const std::string &hexaddr = p.first;
            const JObjectSummary &summary = p.second;
//            std::string obj_json = JJSON_Create(summary);
            std::string obj_json = ObjectToJSON( hexaddr, summary );

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

    //----------------------------------------
    // GetObjectSummaries
    //
    // TODO: Take this and the (same) code in JControlEventPRocessor and move them to a common place
    void GetObjectSummaries(const std::string &factory_name, const std::string &factory_tag, std::map<std::string, JObjectSummary> &objects){
        // bombproof against getting called with no active JEvent
        if(_aEvent.get() == nullptr ) return;
        auto fac = _aEvent->GetFactory(factory_name, factory_tag);
        if( fac == nullptr ){
            jerr << "Unable to find factory specified for prefetching: factory=" << factory_name << " tag=" << factory_tag << std::endl;
            return;
        }
        for( auto jobj : fac->GetAs<JObject>()){
            JObjectSummary summary;
            jobj->Summarize(summary);
            std::stringstream ss;
            ss << "0x" << std::hex << (uint64_t)jobj << std::dec;
            objects[ss.str()] = summary; // key is address of object converted to string
        }
#ifdef HAVE_ROOT
        // For objects inheriting from TObject, we try and convert members automatically
        // into JObjectSummary form. This relies on dictionaries being compiled in.
        // (see ROOT_GENERATE_DICTIONARY for cmake files).
        for( auto tobj : fac->GetAs<TObject>()){
            JObjectSummary summary;
            auto tclass = TClass::GetClass(tobj->ClassName());
            if(tclass){
                auto *members = tclass->GetListOfAllPublicDataMembers();
                for( auto item : *members){
                    TDataMember *memitem = dynamic_cast<TDataMember*>(item);
                    if( memitem == nullptr ) continue;
                    if( memitem->Property() & kIsStatic ) continue; // exclude TObject enums
                    JObjectMember jObjectMember;
                    jObjectMember.name = memitem->GetName();
                    jObjectMember.type = memitem->GetTypeName();
                    jObjectMember.value = GetRootObjectMemberAsString(tobj, memitem, jObjectMember.type);
                    summary.add(jObjectMember);
                }
            }else {
                LOG << "Unable to get TClass for: " << tobj->ClassName() << LOG_END;
            }
            std::stringstream ss;
            ss << "0x" << std::hex << (uint64_t)tobj << std::dec;
            objects[ss.str()] = summary; // key is address of object converted to string
        }
#endif
     }

    //----------------------------------------
    // ObjectToJSON
    std::string ObjectToJSON( const std::string &hexaddr, const JObjectSummary &summary ){

        stringstream ss;
        ss << "{\n";
        ss << "\"hexaddr\":\"" << hexaddr << "\"\n";
        for( auto m : summary.get_fields() ){
            ss << ",\"" << m.name << "\":\"" << m.value << "\"\n";
        }
        ss << "}";

        return std::move(ss.str());
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

    mutex pymutex;

    std::shared_ptr<const JEvent> _aEvent;
    std::map<std::string, std::string> prefetch_factories;
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
