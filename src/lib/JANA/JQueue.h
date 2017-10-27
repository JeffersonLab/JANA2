//
//    File: JQueue.h
// Created: Wed Oct 11 22:51:32 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
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
#ifndef _JQueue_h_
#define _JQueue_h_

#include <cstdint>
#include <string>
#include <atomic>
#include <vector>
#include <mutex>
#include <set>

class JEvent;

class JQueue{
	public:
	
		enum{
			kNone,
			kQUEUE_FULL,
			kNO_ERROR
		}Flags_t;
		
		// The following taken from https://stackoverflow.com/questions/13193484/how-to-declare-a-vector-of-atomic-in-c
		// It allows us to use a vector of atomics
		template <typename T>
		class atomwrapper{
			public:
			  std::atomic<T> _a;
			  atomwrapper():_a(){}
			  atomwrapper(const std::atomic<T> &a):_a(a.load()){}
			  atomwrapper(const atomwrapper &other):_a(other._a.load()){}
			  atomwrapper &operator=(const atomwrapper &other){_a.store(other._a.load());}
			  atomwrapper &operator=(T &other){_a.store(other); return *this;}
		};

	
		JQueue(std::string name, bool run_processors=true);
		virtual ~JQueue();
		
		                   void AddConvertFromType(std::string name);
		                   void AddConvertFromTypes(std::set<std::string> names);
		            virtual int AddEvent(JEvent*);
			                int AddToQueue(JEvent *jevent);
		   const std::set<std::string> GetConvertFromTypes(void);
		               uint32_t GetMaxEvents(void);
		            std::string GetName();
		                JEvent* GetEvent(void);
		               uint32_t GetNumEvents(void);
		               uint64_t GetNumEventsProcessed(void);
		                   bool GetRunProcessors(void);
		
	protected:
		std::string _name;
		bool _run_processors;
		bool _done;
		std::set<std::string> _convert_from_types;
		
		std::vector< JEvent* > _queue;
		std::atomic<uint64_t> _nevents_processed;

		std::atomic<uint32_t> iread;
		std::atomic<uint32_t> iwrite;
		std::atomic<uint32_t> iend;
		std::mutex _mutex;
	private:

};

#endif // _JQueue_h_

