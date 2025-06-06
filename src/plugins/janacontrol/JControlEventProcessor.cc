// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JControlEventProcessor.h"
#include <JANA/JLogger.h>


//-------------------------------------------------------------
// JControlEventProcessor
//-------------------------------------------------------------
JControlEventProcessor::JControlEventProcessor():jstringification(new JStringification) {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}

//-------------------------------------------------------------
// Init
//-------------------------------------------------------------
void JControlEventProcessor::Init() {
    LOG << "JControlEventProcessor::Init" << LOG_END;

    GetApplication()->GetJParameterManager()->SetDefaultParameter("jana:debug_mode", _debug_mode, "Turn on JANA debug mode for janacontrol plugin. (Will require jana-control.py GUI to step through events)");
}

//-------------------------------------------------------------
// Process
//-------------------------------------------------------------
void JControlEventProcessor::Process(const std::shared_ptr<const JEvent> &event) {

    // If _debug_mode is on then stall here until told to move on to next event
    if( _debug_mode ){

        // Serialize threads so setting _wait to true only lets one event through
        static std::mutex mtx;
        std::lock_guard<std::mutex> lck(mtx);

        // Copy pointer to JEvent to internal member so it can be used to probe
        // the event via other methods.
        _jevent = event;

        while(_wait && _debug_mode){
            std::chrono::milliseconds wait_time {100};
            std::this_thread::sleep_for(wait_time);
        }
        _wait=true; // set up to stall on next event

        // Release shared_ptr to JEvent since we are done with it.
        _jevent.reset();
    }

    if( _fetch_flag ) FetchObjects( event );
 }

//-------------------------------------------------------------
// Finish
//-------------------------------------------------------------
void JControlEventProcessor::Finish() {
    // Close any resources
    LOG << "JControlEventProcessor::Finish" << LOG_END;
}

//-------------------------------------------------------------
// SetDebugMode
//-------------------------------------------------------------
void JControlEventProcessor::SetDebugMode(bool debug_mode){
    _debug_mode = debug_mode;
	GetApplication()->SetTimeoutEnabled( !_debug_mode );
}

//-------------------------------------------------------------
// NextEvent
//-------------------------------------------------------------
void JControlEventProcessor::NextEvent(void){
    _wait = false;
}

//-------------------------------------------------------------
// GetObjectStatus
//
// Get count of objects already created for the current event
// for each type of factory.
//-------------------------------------------------------------
void JControlEventProcessor::GetObjectStatus( std::map<JFactorySummary, std::size_t> &factory_object_counts ){

    // bombproof against getting called with no active JEvent
    if(_jevent.get() == nullptr ) return;

    // Get list of all factories associated with this event.
    // n.b. This will also list objects inserted by the event source
    // unlike _jevent->GetJApplication()->GetComponentSummary()
    // which only lists registered factories.
    auto factories = _jevent->GetAllFactories();
    for( auto fac : factories ){
        JFactorySummary fac_info;
        fac_info.plugin_name = fac->GetPluginName();
        fac_info.factory_name = fac->GetFactoryName();
        fac_info.factory_tag = fac->GetTag();
        fac_info.object_name = fac->GetObjectName();
        if(fac_info.factory_name == "") fac_info.factory_name = "JFactoryT<" + fac_info.object_name + ">";
        if( fac ) factory_object_counts[fac_info] = fac->GetNumObjects();
    }
}

//-------------------------------------------------------------
// GetObjects
//
// Get objects for the specified factory in the form of strings.
//-------------------------------------------------------------
void JControlEventProcessor::GetObjects(const std::string &/* factory_name */, const std::string &factory_tag, const std::string &object_name, std::map<std::string, JObjectSummary> &objects){
    // bombproof against getting called with no active JEvent
    if(_jevent.get() == nullptr ) return;

    jstringification->GetObjectSummaries(objects, _jevent, object_name, factory_tag);
}

//-------------------------------------------------------------
// SetFetch
//
// Set list of factories whose objects should be retrieved and packaged
// into strings during the next event.
//-------------------------------------------------------------
void JControlEventProcessor::SetFetchFactories(std::set<std::string> &factorytags){

    _fetch_object_summaries.clear();
    _fetch_metadata.clear();
    _fetch_factorytags = factorytags;
    _fetch_flag = true;
}

//-------------------------------------------------------------
// FetchObjects
//
/// Fetch the objects set in the fetch list by an earlier call to
/// SetFetch(). Objects will be retrieved from the given event
/// and JObjectSummary objects created for them and stored in
/// the _fetch_object_summaries container. The _fetch_flag
/// will be set to false once the objects in _fetch_object_summaries
/// are ready.
//-------------------------------------------------------------
void JControlEventProcessor::FetchObjects(std::shared_ptr<const JEvent> jevent){

    _fetch_object_summaries.clear(); // should be redundant with clear in SetFetchFactories()
    _fetch_metadata.clear();

    _fetch_metadata["run_number"]   = ToString( GetRunNumber());
    _fetch_metadata["event_number"] = ToString( GetEventNumber());

    // Loop over factories set in a previous call to SetFetchFactories()
    for( const auto& factorytag : _fetch_factorytags){
        // Add entry to _fetch_object_summaries and get reference to it
        auto &objects = _fetch_object_summaries[factorytag];

        // Split factorytag string into factory part and tag part
        auto pos = factorytag.find(":");
        std::string factory_name = factorytag.substr(0, pos);
        std::string tag = (pos==std::string::npos ? "":factorytag.substr(pos+1));

        jstringification->GetObjectSummaries(objects, jevent, factory_name, tag);
    }

    // All done. Notify JControlZMQ::FetchJANAObjectsJSON
    _fetch_flag = false;
}

//-------------------------------------------------------------
// FetchObjectsNow
//
/// This is a wrapper to FetchObjects that can be called while in
/// debug mode (i.e. when event processing is stalled). It allows
/// JControlZMQ::FetchJANAObjectsJSON to get objects from the
/// current event while in debug_mode. Like FetchObjects(), this
/// assumes SetFetch() has already been called. This will set the
/// _fetch_flag to false when done and should be followed by a
/// call to GetLastFetchResult() to get the actual results.
//-------------------------------------------------------------
void JControlEventProcessor::FetchObjectsNow(void){
    // bombproof against getting called with no active JEvent
    if(_jevent.get() == nullptr ) return;

    FetchObjects( _jevent );
}

//-------------------------------------------------------------
// GetRunNumber
//-------------------------------------------------------------
uint32_t JControlEventProcessor::GetRunNumber(void){
    if(_jevent.get() == nullptr ) return 0;
    return _jevent->GetRunNumber();
}

//-------------------------------------------------------------
// GetEventNumber
//-------------------------------------------------------------
uint64_t JControlEventProcessor::GetEventNumber(void){
    if(_jevent.get() == nullptr ) return 0;
    return _jevent->GetEventNumber();
}
