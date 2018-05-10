//
//    File: JFactorySet.cc
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

#include <iterator>
#include <iostream>

#include "JApplication.h"
#include "JFactorySet.h"
#include "JFactory.h"
#include "JFactoryGenerator.h"

//---------------------------------
// JFactorySet    (Constructor)
//---------------------------------
JFactorySet::JFactorySet(void)
{

}

//---------------------------------
// JFactorySet    (Constructor)
//---------------------------------
JFactorySet::JFactorySet(const std::vector<JFactoryGenerator*>& aFactoryGenerators)
{
	//Add all factories from all factory generators
	for(auto sGenerator : aFactoryGenerators){

		// Generate the factories into a temporary JFactorySet.
		JFactorySet myset;
		sGenerator->GenerateFactories( &myset );

		// Merge factories from temporary JFactorySet into this one. Any that
		// already exist here will leave the duplicates in the temporary set
		// where they will be destroyed by its destructor as it falls out of scope.
		Merge( myset );
	}
}

//---------------------------------
// ~JFactorySet    (Destructor)
//---------------------------------
JFactorySet::~JFactorySet()
{
	/// The destructor will delete any factories in the set.
	for( auto f : mFactories ) delete f.second;
}

//---------------------------------
// Add
//---------------------------------
bool JFactorySet::Add(JFactory* aFactory)
{
	/// Add a factory to this JFactorySet. The caller should be careful when
	/// using the factory pointer after calling this since the factory may be deleted
	/// during this call. This will happen if a factory of the same type and tag
	/// already exists in this set. A return value of "true" indicates that the
	/// factory was successfully added and that the given pointer is still valid
	/// (until this set is destroyed). A return value of "false" indicates that
	/// this factory was a duplicate and therefore the factory object was destroyed.
	/// In all cases, this assumes ownership of the factory object.

	auto sKey = std::make_pair( aFactory->GetObjectType(), aFactory->GetTag() );
	auto res = mFactories.emplace(sKey, static_cast<JFactory*>(aFactory));
	if( res.second ) return true; // factory successfully added
	
	// factory is duplicate. Destroy it and inform caller by returning false.
	delete aFactory;
	
	return false;
}

//---------------------------------
// GetFactory
//---------------------------------
JFactory* JFactorySet::GetFactory(std::type_index aObjectType, const std::string& aFactoryTag) const
{
	auto sKeyPair = std::make_pair(aObjectType, aFactoryTag);
	auto sIterator = mFactories.find(sKeyPair);
	return (sIterator != std::end(mFactories)) ? sIterator->second : nullptr;
}

//---------------------------------
// Merge
//---------------------------------
void JFactorySet::Merge(JFactorySet &aFactorySet)
{
	/// Merge any factories in the specified JFactorySet into this
	/// one. Any factories which don't have the same type and tag as one
	/// already in this set will be transferred and this JFactorySet
	/// will take ownership of them. Ones that have a type and tag
	/// that matches one already in this set will be left in the
	/// original JFactorySet. Thus, all factories left in the JFactorySet
	/// passed into this method upon return from it can be considered
	/// duplicates. It will be left to the caller to delete those.
	
	JFactorySet tmpSet; // keep track of duplicates to copy back into aFactorySet
	for( auto f : aFactorySet.mFactories ){
		if( ! mFactories.insert( f ).second ) {
			// duplicate. Record so we can send back to caller
			tmpSet.mFactories[f.first] = f.second;
		}
	}
	
	// Copy duplicates back to aFactorySet
	aFactorySet.mFactories.swap( tmpSet.mFactories );
	tmpSet.mFactories.clear(); // prevent ~JFactorySet from deleting any factories
}

//---------------------------------
// Print
//---------------------------------
void JFactorySet::Print(void) const
{
	size_t max_len = 0;
	for( auto p: mFactories ){
		auto len = p.second->GetName().length();
		if( len > max_len ) max_len = len;
	}

	max_len += 4;
	for( auto p: mFactories ){
		auto name = p.second->GetName();
		auto tag = p.second->GetTag();
		
		std::cout << std::string( max_len-name.length(), ' ') + name;
		if(!tag.empty()) std::cout << ":" << tag;
		std::cout << std::endl;
	}
}

//---------------------------------
// Release
//---------------------------------
void JFactorySet::Release(void)
{
	//Loop over all factories and: clear the data and set the created flag to false
	for(const auto& sFactoryPair : mFactories){
		auto sFactory = sFactoryPair.second;
		sFactory->ClearData();
		sFactory->SetCreated(false);
	}
}
