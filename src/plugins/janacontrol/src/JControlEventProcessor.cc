// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JControlEventProcessor.h"
#include <JANA/JLogger.h>

#include <sstream>

#ifdef HAVE_ROOT
#include <TObject.h>
#include <TClass.h>
#include <TDataMember.h>
#include <TMethodCall.h>
#include <Tlist.h>

/// Helper routines to extract value of member of TObject.

// These 4 templates use SFINAE to allow the GetAddrAsString template below to compile
// even when the template argument is something like a string that cannot be cast to
// an (int) or (unsigned int). We need this because we want to treat char and unsigned
// char variables as numbers and not characters.
template <typename T> void ConvertInt(std::stringstream &ss, T val, std::true_type){ss << (int)val;}
template <typename T> void ConvertInt(std::stringstream &ss, T val, std::false_type){ss << "unknown";}
template <typename T> void ConvertUInt(std::stringstream &ss, T val, std::true_type){ss << "0x" << std::hex << (unsigned int)val << std::dec;}
template <typename T> void ConvertUInt(std::stringstream &ss, T val, std::false_type){ss << "unknown";}

/// The GetAddrAsString template is used to convert an untyped addr
/// into a std::string. It interprets the address as pointing to
/// a primitive object of the same type as the template argument.
// This is done in a very C-style way by doing address arithmetic.
// There is almost certainly a better way to do this, but the
// ROOT documentation is not forthcoming.
template <typename T>
std::string GetAddrAsString(void *addr){
    auto val = *(T *)addr;
    std::stringstream ss;
    if( std::is_same<T, char>::value ) { // darn char types have to be treated special!
        ConvertInt(ss, val, std::is_same<T, char>());
    }else if( std::is_same<T, unsigned char>::value ){
        ConvertUInt(ss, val, std::is_same<T, unsigned char>());
    }else if( std::is_same<T, std::string>::value ){
        ss  << val;
    }else{
        ss << val;
    }
    return ss.str();
}

/// GetRootObjectMemberAsString is the entry point for converting members of
/// TObject derived objects into strings. This really only works for a few
/// primitive types, but is useful for debugging/viewing single events.
std::string GetRootObjectMemberAsString(const TObject *tobj, const TDataMember *memitem, const std::string &type){
    void *addr = (void*)(((uint64_t)tobj) + memitem->GetOffset()); // untyped address of data member
    if(      type == "int"           ) return GetAddrAsString<int>(addr);
    else if( type == "double"        ) return GetAddrAsString<double>(addr);
    else if( type == "float"         ) return GetAddrAsString<float>(addr);
    else if( type == "char"          ) return GetAddrAsString<char>(addr);
    else if( type == "unsigned char" ) return GetAddrAsString<unsigned char>(addr);
    else if( type == "unsigned int"  ) return GetAddrAsString<unsigned int>(addr);
    else if( type == "long"          ) return GetAddrAsString<long>(addr);
    else if( type == "unsigned long" ) return GetAddrAsString<unsigned long>(addr);
    else if( type == "ULong64_t"     ) return GetAddrAsString<ULong64_t>(addr);
    else if( type == "Long64_t"      ) return GetAddrAsString<Long64_t>(addr);
    else if( type == "string"        ) return GetAddrAsString<std::string>(addr);
    return "unknown";
}
#endif // HAVE_ROOT


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
                LOG << "Unable to get TClass for: " << object_name << LOG_END;
            }
            std::stringstream ss;
            ss << "0x" << std::hex << (uint64_t)tobj << std::dec;
            objects[ss.str()] = summary; // key is address of object converted to string
        }
#endif
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