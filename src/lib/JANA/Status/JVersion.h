//
//    File: JVersion.h
// Created: Wed Oct 11 13:26:41 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date: Wed Oct 25 23:19:03 2017 -0400 ]
// [ Author: David Lawrence <davidl@jlab.org> ]
// [ Source: src/lib/JANA/JVersion.h ]
// [ Revision: 28bf59642adb3d82f0cc3bd6405279076bf8f1e6 ]
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Jefferson Science Associates LLC Copyright Notice:  
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:  
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer. 
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.  
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.  
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.   
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 

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
		      static string GetDevStatus(void){return string("dev");} // return either "dev" or ""
		
		static string GetVersion(void){
			std::stringstream ss;
			ss<<major<<"."<<minor<<"."<<build<<GetDevStatus();
			return ss.str();
		}
		
		static string GetIDstring(void){return "[ Id: JVersion.h | Wed Oct 25 23:19:03 2017 -0400 | David Lawrence  ]";}
		static string GetRevision(void){return ExtractContent("[ Revision: 28bf59642adb3d82f0cc3bd6405279076bf8f1e6 ]");}
		static string GetDate(void){return ExtractContent("[ Date: Wed Oct 25 23:19:03 2017 -0400 ]");}
		static string GetSource(void){return ExtractContent("[ Source: src/lib/JANA/JVersion.h ]");}
		
	protected:
		
		static string ExtractContent(const char *ccstr){
			/// Attempt to cut off everything leading up to and including
			/// the first space character and everything after and
			/// including the last space character of the given string.
			string str = ccstr;
			string::size_type pos_start = str.find_first_of(" ", 0);
			if(pos_start==string::npos)return str;
			string::size_type pos_end = str.find_last_of(" ", str.size());
			if(pos_end==string::npos)return str;
			return str.substr(pos_start+1, pos_end-(pos_start+1));
		}
};

#endif // _JVersion_h_

