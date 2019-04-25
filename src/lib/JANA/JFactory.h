//
//    File: JFactory.h
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

#include <string>
#include <typeindex>
#include <memory>
#include <limits>
#include <atomic>
#include <vector>

class JEvent;
class JObject;

#ifndef _JFactory_h_
#define _JFactory_h_

class JFactory
{
	public:

		enum JFactory_Flags_t{
			JFACTORY_NULL		=0x00,
			PERSISTANT			=0x01,
			WRITE_TO_OUTPUT	=0x02,
			NOT_OBJECT_OWNER	=0x04
		};

		JFactory(std::string aName, std::string aTag = "") : mName(aName), mTag(aTag) { };
		virtual ~JFactory() = default;

		std::string GetName(void) const{return mName;}
		std::string GetTag(void) const{return mTag;}

		//VIRTUAL METHODS OVERLOADED BY JFactoryT
		virtual std::type_index GetObjectType(void) const = 0;
		virtual void ClearData(void) = 0;

		//VIRTUAL METHODS TO BE OVERLOADED BY USER FACTORIES
		virtual void Init(void){}
		virtual void ChangeRun(const std::shared_ptr<const JEvent>& aEvent){}
		virtual void Process(const std::shared_ptr<const JEvent>& aEvent){}
//		virtual void Create(const std::shared_ptr<const JEvent>& aEvent){}

		uint32_t GetPreviousRunNumber(void) const{return mPreviousRunNumber;}
		void SetPreviousRunNumber(uint32_t aRunNumber){mPreviousRunNumber = aRunNumber;}

		//Have objects already been created?
		bool GetCreated(void) const{return mCreated;}
		void SetCreated(bool aCreated){mCreated = aCreated;}

		bool AcquireCreatingLock(void);
		const std::atomic<bool>& GetCreatingLock(void) const;
		void ReleaseCreatingLock(void);

		// Copy/Move objects into factory
		template<typename T>
		void Set(std::vector<T*>& items) {
		    for (T* item : items) {
		    	Insert(item);
		    }
		}
		// Another option:
		// Get rid of Set(std::vector<JObject*>& items) completely.
		// Have virtual Set<T>(std::vector<T*>& items) { throw JException("Wrong type!"); }
		// When T matches JFactoryT<T>, then this dispatches to the JFactoryT<T>::Set()
		// The main downside I see right now is a potentially huge vtable

		/// Get all flags in the form of a single word
		inline uint32_t GetFactoryFlags(void){return mFlags;}
	
		/// Set a flag (or flags)
		inline void SetFactoryFlag(JFactory_Flags_t f){
			mFlags |= (uint32_t)f;
		}

		/// Clear a flag (or flags)
		inline void ClearFactoryFlag(JFactory_Flags_t f){
			mFlags &= ~(uint32_t)f;
		}

		/// Test if a flag (or set of flags) is set
		inline bool TestFactoryFlag(JFactory_Flags_t f){
			return (mFlags & (uint32_t)f) == (uint32_t)f;
		}

		// Used to make sure Init is called only once
		std::once_flag init_flag; 

	protected:
		virtual void Set(std::vector<JObject*>& data) = 0;
		virtual void Insert(JObject* data) = 0;

		std::string mName;
		std::string mTag;
		uint32_t    mFlags;
		std::atomic<bool> mCreating{false}; //true if a thread is currently creating objects (effectively a lock)
		std::atomic<bool> mCreated{false}; //true if created previously, false if not
		uint32_t mPreviousRunNumber = 0;

};

inline const std::atomic<bool>& JFactory::GetCreatingLock(void) const
{
	//Get a reference to the lock. This is so that threads can do other things in the meantime while waiting for this
	return mCreating;
}

inline bool JFactory::AcquireCreatingLock(void)
{
	bool sExpected = false;
	return mCreating.compare_exchange_weak(sExpected, true);
}

inline void JFactory::ReleaseCreatingLock(void)
{
	//Only call with thread used to acquire!
	mCreating = false;
}

#endif // _JFactory_h_

