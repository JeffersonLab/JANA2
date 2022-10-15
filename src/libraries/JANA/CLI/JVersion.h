
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JVersion_h_
#define _JVersion_h_

#include <sstream>

struct JVersion {

    static const int major = 2; //@jana2_VERSION_MAJOR@;
    static const int minor = 0; //@jana2_VERSION_MINOR@;
    static const int patch = 7; //@jana2_VERSION_PATCH@;
    static const bool release = false; //@JVERSION_RELEASE@;
    // static const char* git_hash = "none"; //@JVERSION_GIT_HASH@;
    // static const char* last_built = "today"; //@JVERSION_LAST_BUILT@;

    static unsigned int GetMajor(){ return major; }
    static unsigned int GetMinor(){ return minor; }
    static unsigned int GetBuild(){ return patch; }

    static std::string GetIDstring(){ return "Id: JVersion.h | Wed Oct 25 23:19:03 2017 -0400 | David Lawrence"; }
    static std::string GetRevision(){ return "Revision: 28bf59642adb3d82f0cc3bd6405279076bf8f1e6"; }
    static std::string GetDate(){ return "Date: Wed Oct 25 23:19:03 2017 -0400"; }
    static std::string GetSource(){ return "Source: src/libraries/JANA/JVersion.h ]";}

    static std::string GetVersion() {
        std::stringstream ss;
        ss << major << "." << minor << "." << patch << " (" << (release ? "Release" : "Snapshot") << ")";
        return ss.str();
    }
};

#endif // _JVersion_h_

