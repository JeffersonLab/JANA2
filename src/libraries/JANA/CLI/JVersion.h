
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JVersion_h_
#define _JVersion_h_

#include <sstream>

class JVersion{
	public:
		JVersion();
		virtual ~JVersion();
		enum{
			major = 2,
			minor = 0,
			build = 0
		};
		
		static unsigned int GetMajor(void){return major;}
		static unsigned int GetMinor(void){return minor;}
		static unsigned int GetBuild(void){return build;}
		      static std::string GetDevStatus(void){return std::string("dev");} // return either "dev" or ""
		
		static std::string GetVersion(void){
			std::stringstream ss;
			ss<<major<<"."<<minor<<"."<<build<<GetDevStatus();
			return ss.str();
		}
		
		static std::string GetIDstring(void){return "[ Id: JVersion.h | Wed Oct 25 23:19:03 2017 -0400 | David Lawrence  ]";}
		static std::string GetRevision(void){return ExtractContent("[ Revision: 28bf59642adb3d82f0cc3bd6405279076bf8f1e6 ]");}
		static std::string GetDate(void){return ExtractContent("[ Date: Wed Oct 25 23:19:03 2017 -0400 ]");}
		static std::string GetSource(void){return ExtractContent("[ Source: src/libraries/JANA/JVersion.h ]");}
		
	protected:
		
		static std::string ExtractContent(const char *ccstr){
			/// Attempt to cut off everything leading up to and including
			/// the first space character and everything after and
			/// including the last space character of the given string.
			std::string str = ccstr;
			std::string::size_type pos_start = str.find_first_of(" ", 0);
			if(pos_start==std::string::npos)return str;
			std::string::size_type pos_end = str.find_last_of(" ", str.size());
			if(pos_end==std::string::npos)return str;
			return str.substr(pos_start+1, pos_end-(pos_start+1));
		}
};

#endif // _JVersion_h_

