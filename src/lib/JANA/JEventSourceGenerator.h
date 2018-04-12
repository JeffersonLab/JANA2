//
//    File: JEventSourceGenerator.h
// Created: Thu Oct 12 08:15:45 EDT 2017
// Creator: davidl (on Darwin harriet.jlab.org 15.6.0 i386)
//
// ------ Last repository commit info -----
// [ Date ]
// [ Author ]
// [ Source ]
// [ Revision ]
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
//
// Description:
//
//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
#ifndef _JEventSourceGenerator_h_
#define _JEventSourceGenerator_h_

#include <string>

#include <JEventSource.h>
#include "JQueueInterface.h"

/// This is a base class for all event source generators. JANA implements
/// event sources in a modular way so that new types of sources can be
/// easily added. Typically, this just means different file formats, but
/// an also be networked or shared memory sources. To provide a new
/// source type, a class must inherit from JEventSourceGenerator and
/// implement the three virtual methods it defines:
///
/// Description:
/// 	This method should just return a short string that can be used to
/// 	inform the user what type of source this implements. It is purely
/// 	informational in that the framework never looks at its contents.
///
/// CheckOpenable:
/// 	This method should "peek" at the source to see if it is one that
/// 	it can open. It should return a value between 0 and 1 inclusive
/// 	with 0 meaning "cannot open" and 1 meaning "absolutely can open".
/// 	The check can be as simple as looking at the source name (e.g.
/// 	does it have a specific suffix) or more involved (e.g. opening
/// 	the file and checking for a magic header). Note that it should
/// 	not be assumed that the source represents a file name. It could
/// 	indicate a URL or how to connect to some other non-file source.
///
/// MakeJEventSource:
/// 	This will be called when the value of CheckOpenable returned
/// 	by this object is larger than that returned by all other
/// 	JEventSourceGenerator objects. This should simply instantiate
/// 	an object of the JEventSource based class that does the actual
/// 	work of reading objects from the source.

class JEventSourceGenerator{
	public:
		JEventSourceGenerator(std::string name);
		virtual ~JEventSourceGenerator();
		
		virtual const char* Description(void)=0; ///< Get string indicating type of source this handles
		virtual double CheckOpenable(std::string source)=0; ///< Test probability of opening the given source
		virtual JEventSource* MakeJEventSource(std::string source)=0; ///< Instantiate an JEventSource object (subclass)

		std::string GetName(void) const;
		
	protected:
		std::string _name;
};

#endif // _JEventSourceGenerator_h_

