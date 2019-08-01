//
//    File: JEventSource.cc
// Created: Thu Oct 12 08:15:39 EDT 2017
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

#include <JANA/JEventSource.h>
#include <JANA/JApplication.h>
#include <JANA/JEvent.h>
#include <JANA/JTypeInfo.h>

//---------------------------------
// JEventSource    (Constructor)
//---------------------------------
JEventSource::JEventSource(string name, JApplication* aApplication) : mApplication(aApplication), mName(name)
{
	/// Do not open the source here!
	/// It will be opened in the Open() method
	///
	/// If anything more than the default JEventQueue structure is needed then create it here.
	/// You may also add JFactoryGenerator objects to the JApplication to generate factories
	/// for holding object types created by this source.
}

//---------------------------------
// ~JEventSource    (Destructor)
//---------------------------------
JEventSource::~JEventSource()
{

}

//---------------------------------
// Open
//---------------------------------
void JEventSource::Open(void)
{

}

//---------------------------------
// SetJApplication
//---------------------------------
void JEventSource::SetJApplication(JApplication *app)
{
	mApplication = app;
}

//---------------------------------
// GetType
//---------------------------------
std::string JEventSource::GetType(void) const
{
	return JTypeInfo::demangle<decltype(*this)>();
}


//---------------------------------
// SetNumEventsToGetAtOnce
//---------------------------------
void JEventSource::SetNumEventsToGetAtOnce(std::size_t aMinNumEvents, std::size_t aMaxNumEvents)
{
	mMinNumEventsToGetAtOnce = aMinNumEvents;
	mMaxNumEventsToGetAtOnce = aMaxNumEvents;
}

//---------------------------------
// GetNumEventsToGetAtOnce
//---------------------------------
std::pair<std::size_t, std::size_t> JEventSource::GetNumEventsToGetAtOnce(void) const
{
	return std::make_pair(mMinNumEventsToGetAtOnce, mMaxNumEventsToGetAtOnce);
}


//---------------------------------
// IsExhausted
//---------------------------------
bool JEventSource::IsExhausted(void) const
{
	return mExhausted;
}



