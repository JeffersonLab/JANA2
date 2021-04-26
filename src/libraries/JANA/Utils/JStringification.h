
// Copyright 2021, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JSTRINGIFICATION_H
#define JSTRINGIFICATION_H

#include <sstream>

#ifdef HAVE_ROOT
#include <TObject.h>
#include <TClass.h>
#include <TDataMember.h>
#include <TMethodCall.h>
#include <TList.h>
#endif // HAVE_ROOT

#include <JANA/JObject.h>
#include <JANA/JEvent.h>

class JStringification {

public:

    JStringification() = default;
    virtual ~JStringification() = default;

    // These are the main entry points that users will want to use
    void GetObjectSummaries(std::map<std::string, JObjectSummary> &objects, std::shared_ptr<const JEvent> &jevent, const std::string &object_name, const std::string factory_tag="") const;
    void GetObjectSummariesAsJSON(std::vector<std::string> &json_vec, std::shared_ptr<const JEvent> &jevent, const std::string &object_name, const std::string factory_tag="") const;
    std::string ObjectToJSON( const std::string &hexaddr, const JObjectSummary &summary ) const;

    //-----------------------------------------------------------------------------------

    // These 4 templates use SFINAE to allow the GetAddrAsString template below to compile
    // even when the template argument is something like a string that cannot be cast to
    // an (int) or (unsigned int). We need this because we want to treat char and unsigned
    // char variables as numbers and not characters.
    template <typename T> void ConvertInt(std::stringstream &ss, T val, std::true_type) const {ss << (int)val;}
    template <typename T> void ConvertInt(std::stringstream &ss, T val, std::false_type) const {ss << "unknown";}
    template <typename T> void ConvertUInt(std::stringstream &ss, T val, std::true_type) const {ss << "0x" << std::hex << (unsigned int)val << std::dec;}
    template <typename T> void ConvertUInt(std::stringstream &ss, T val, std::false_type) const {ss << "unknown";}

    template <typename T> std::string GetAddrAsString(void *addr) const;

#ifdef HAVE_ROOT
    std::string GetRootObjectMemberAsString(const TObject *tobj, const TDataMember *memitem, std::string type) const;
#endif // HAVE_ROOT

private:


};

/// The GetAddrAsString template is used to convert an untyped addr
/// into a std::string. It interprets the address as pointing to
/// a primitive object of the same type as the template argument.
// This is done in a very C-style way by doing address arithmetic.
// There is almost certainly a better way to do this, but the
// ROOT documentation is not forthcoming.
template <typename T>
std::string JStringification::GetAddrAsString(void *addr) const {
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

#endif //JSTRINGIFICATION_H