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

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceManager.h>
#include <JANA/JQueue.h>

/// This is a base class for all event source generators. JANA implements
/// event sources in a modular way so that new types of sources can be
/// easily added. Typically, this just means different file formats, but
/// can also be networked or shared memory sources.
///
/// One may subclass the JEventSource class directly, but it is recommended
/// to use the template class JEventSourceGeneratorT instead. That defines
/// defaults for the required methods based on the JEventSource class
/// the generator is for while still allowing to customization where
/// needed. See the JEventSourceGeneratorT documentation for details.

class JEventSourceGenerator{
	public:
	
		friend JEventSourceManager;
	
		JEventSourceGenerator(JApplication *app=nullptr):mApplication(app){}
		virtual ~JEventSourceGenerator(){}

		// Default versions of these are defined in JEventSourceGeneratorT.h
		virtual std::string GetType(void) const = 0; ///< Return name of the source type this will generate
		virtual std::string GetDescription(void) const = 0; ///< Return description of the source type this will generate
		virtual JEventSource* MakeJEventSource( std::string source ) = 0; ///< Create an instance of the source type this generates
		virtual double CheckOpenable( std::string source ) = 0; ///< See JEventSourceGeneratorT for description
	
	protected:
	
		/// This is called by JEventSourceManager::AddJEventSourceGenerator which
		/// itself is called by JApplication::Add(JEventSourceGenerator*). There
		/// should be no need to call it from anywhere else.
		void SetJApplication(JApplication *app){ mApplication = app; }

		JApplication* mApplication{nullptr};
};

#endif // _JEventSourceGenerator_h_

