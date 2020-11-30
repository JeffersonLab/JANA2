
#include "JControlEventProcessor.h"
#include <JANA/JLogger.h>

//-------------------------------------------------------------
// JControlEventProcessor
//-------------------------------------------------------------
JControlEventProcessor::JControlEventProcessor(JApplication *japp):JEventProcessor(japp) {
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}

//-------------------------------------------------------------
// Init
//-------------------------------------------------------------
void JControlEventProcessor::Init() {
    LOG << "JControlEventProcessor::Init" << LOG_END;
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

    // Get a JComponentSummary which contains a list of factory names/tags.
    auto jcs = _jevent->GetJApplication()->GetComponentSummary();

    // Loop over factories and get number of existing objects for this event for each
    for( auto fac_info : jcs.factories ){
        auto fac = _jevent->GetFactory(fac_info.object_name, fac_info.factory_tag);
        if( fac ) factory_object_counts[fac_info] = fac->GetNumObjects();
    }
}

//-------------------------------------------------------------
// GetObjects
//
// Get objects for the specified factory in the form of strings.
//-------------------------------------------------------------
void JControlEventProcessor::GetObjects(const std::string &factory_name, const std::string &factory_tag, const std::string &object_name, std::map<std::string, JObjectSummary> &objects){
    // bombproof against getting called with no active JEvent
    if(_jevent.get() == nullptr ) return;
    auto fac = _jevent->GetFactory(object_name, factory_tag);
    if( fac ){
        for( auto jobj : fac->GetAs<JObject>()){
            JObjectSummary summary;
            jobj->Summarize(summary);
            std::stringstream ss;
            ss << "0x" << std::hex << (uint64_t)jobj << std::dec;
            objects[ss.str()] = summary; // key is address of object converted to string
        }
    }else{
        _DBG_<<"No factory found! object_name=" << object_name << std::endl;
    }
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