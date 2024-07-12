
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include "JStringification.h"

//-------------------------------------------------------------
// GetObjectSummariesAsJSON
//
/// Get objects for the specified factory in the form of
/// JSON formatted strings.
//-------------------------------------------------------------
void JStringification::GetObjectSummariesAsJSON(std::vector<std::string> &json_vec, std::shared_ptr<const JEvent> &jevent, const std::string &object_name, const std::string factory_tag) const{

    // Get object from the given JEvent from the specified factory
    std::map<std::string, JObjectSummary> objects;
    GetObjectSummaries(objects, jevent, object_name, factory_tag);

    // Loop over objects and create a JSON string out of each
    for(auto p : objects){
        const std::string &hexaddr = p.first;
        JObjectSummary &summary = p.second;
        auto str = ObjectToJSON(hexaddr, summary);
        json_vec.push_back(str);
    }
}

//-------------------------------------------------------------
// GetObjects
//
/// Get objects for the specified factory in the form of strings
/// organized inside JObjectSummary objects.
//-------------------------------------------------------------
void JStringification::GetObjectSummaries(std::map<std::string, JObjectSummary> &objects, std::shared_ptr<const JEvent> &jevent, const std::string &object_name, const std::string factory_tag) const {

    /// Get objects for the specified factory in the form of strings.

    // bombproof against getting called with no active JEvent
    if(jevent.get() == nullptr ) return;
    auto fac = jevent->GetFactory(object_name, factory_tag);
    if( fac ){
        for( auto jobj : fac->GetAs<JObject>()){
            JObjectSummary summary;
            jobj->Summarize(summary);
            std::stringstream ss;
            ss << "0x" << std::hex << (uint64_t)jobj << std::dec;
            objects[ss.str()] = summary; // key is address of object converted to string
        }
#if JANA2_HAVE_ROOT
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
// ObjectToJSON
//
/// Convert a given JObjectSummary into a JSON formatted string.
/// An additiona "hexaddr" member will be added with the value
/// passed in to this method. This is meant to contain the address
/// of the actual object the JObjectSummary represents.
//-------------------------------------------------------------
std::string JStringification::ObjectToJSON( const std::string &hexaddr, const JObjectSummary &summary ) const {

    std::stringstream ss;
    ss << "{\n";
    ss << "\"hexaddr\":\"" << hexaddr << "\"\n";
    for( auto m : summary.get_fields() ){
        ss << ",\"" << m.name << "\":\"" << m.value << "\"\n";
    }
    ss << "}";

    return ss.str();
}

//-------------------------------------------------------------
// GetRootObjectMemberAsString
//-------------------------------------------------------------
#if JANA2_HAVE_ROOT
/// GetRootObjectMemberAsString is the entry point for converting members of
/// TObject derived objects into strings. This really only works for a few
/// primitive types, but is useful for debugging/viewing single events.
std::string JStringification::GetRootObjectMemberAsString(const TObject *tobj, const TDataMember *memitem, std::string type) const {
    void *addr = (void*)(((uint64_t)tobj) + memitem->GetOffset()); // untyped address of data member

    // Convert char arrays to std:string so they display properly (assume that is what user wants)
    std::string tmp;
    if( (memitem->Property()&kIsArray) && (type=="char") ){
        tmp = (char*)addr;
        addr = (void*)&tmp;
        type = "string";
    }

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
#endif // JANA2_HAVE_ROOT