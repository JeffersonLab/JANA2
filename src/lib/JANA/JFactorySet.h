//
//    File: JFactorySet.h
// Created: Fri Oct 20 09:33:40 EDT 2017
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
#ifndef _JFactorySet_h_
#define _JFactorySet_h_

#include <string>
#include <typeindex>
#include <map>

#include "JResettable.h"
#include "JFactoryT.h"

class JFactoryGenerator;
class JFactory;


class JFactorySet : public JResettable
{
	public:
		JFactorySet(void);
		JFactorySet(const std::vector<JFactoryGenerator*>& aFactoryGenerators);
		virtual ~JFactorySet();
		
		bool Add(JFactory* aFactory);
		void Merge(JFactorySet &aFactorySet);
		void Print(void) const;
		void Release(void);

		JFactory* GetFactory(std::type_index aObjectType, const std::string& aFactoryTag="") const;
		template<typename T> JFactoryT<T>* GetFactory(const std::string& tag = "") const;

	protected:
	
		//string: tag
		std::map<std::pair<std::type_index, std::string>, JFactory*> mFactories;
};

template<typename T>
JFactoryT<T>* JFactorySet::GetFactory(const std::string& tag) const {

	auto sKeyPair = std::make_pair(std::type_index(typeid(T)), tag);
	auto sIterator = mFactories.find(sKeyPair);
	return (sIterator != std::end(mFactories)) ? static_cast<JFactoryT<T>*>(sIterator->second) : nullptr;
}

#endif // _JFactorySet_h_

