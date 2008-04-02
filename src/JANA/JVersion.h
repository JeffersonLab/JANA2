// $Id$
//
//    File: JVersion.h
// Created: Wed Apr  2 12:39:36 EDT 2008
// Creator: davidl (on Darwin fwing-dhcp13.jlab.org 8.11.1 i386)
//

#ifndef _JVersion_
#define _JVersion_

#include <string>
#include <sstream>


class JVersion{
	public:
		JVersion();
		virtual ~JVersion();
		virtual const char* className(void){return static_className();}
		static const char* static_className(void){return "JVersion";}
		
		enum{
			major = 0,
			minor = 3,
			build = 0
		};
		
		static unsigned int GetMajor(void){return major;}
		static unsigned int GetMinor(void){return minor;}
		static unsigned int GetBuild(void){return build;}
		
		static string GetVersion(void){
			std::stringstream ss;
			ss<<major<<"."<<minor<<"."<<build;
			return ss.str();
		}
		
		static string GetIDstring(void){
			string id = "$Id$";
			return id;
		}

		static string GetSVNrevision(void){
			string id = "$Id$";
			return id;
		}
		
	protected:
		
	
	private:

};

#endif // _JVersion_

