//
//    File: JFactoryBase.h
// Created: Fri Oct 20 09:44:48 EDT 2017
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
#ifndef _JFactoryBase_h_
#define _JFactoryBase_h_

#include <string>
#include <typeindex>
#include <memory>
#include <limits>

class JEvent;

class JFactoryBase
{
	public:

		JFactoryBase(std::string aName, std::string aTag = "") : mName(aName), mTag(aTag) { };
		virtual ~JFactoryBase() = default;

		std::string GetName(void) const;
		std::string GetTag(void) const;
		std::type_index GetObjectType(void) const = 0;

		virtual void Create(const std::shared_ptr<JEvent>& aEvent){};
		virtual void ChangeRun(const std::shared_ptr<JEvent>& aEvent){};

		uint32_t GetPreviousRunNumber(void) const{return mPreviousRunNumber;}
		void SetPreviousRunNumber(uint32_t aRunNumber){mPreviousRunNumber = aRunNumber;}

		//Have objects already been created?
		bool GetCreated(void) const{return mCreated;}
		void SetCreated(bool aCreated){mCreated = aCreated;}

		bool AcquireCreatingLock(void);
		void ReleaseCreatingLock(void);

		virtual void ClearData(void) = 0;

	private:
		std::string mName;
		std::string mTag;
		std::atomic<bool> mCreating{false}; //true if a thread is currently creating objects (effectively a lock)
		std::atomic<bool> mCreated{false}; //true if created previously, false if not
		uint32_t mPreviousRunNumber = std::numeric_limits<uint32_t>::max();
};

#endif // _JFactoryBase_h_

inline bool JFactoryBase::AcquireCreatingLock(void)
{
	bool sExpected = false;
	return mCreating.compare_exchange_weak(sExpected, true);
}

inline void JFactoryBase::ReleaseCreatingLock(void)
{
	//Only call with thread used to acquire!
	mCreating = false;
}
