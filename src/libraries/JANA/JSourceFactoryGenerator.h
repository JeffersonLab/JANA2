//
//    File: JSourceFactoryGenerator.h
// Created: Thu Oct 12 08:15:55 EDT 2017
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
#ifndef _JSourceFactoryGenerator_h_
#define _JSourceFactoryGenerator_h_

#include <vector>
#include <iterator>

#include "JFactoryGenerator.h"
#include "JFactory.h"

//This class is used to generate generic factories for the given types.
//Generic: JFactory<DataType> factories with no tags or internal object construction.
//This is intended to be used by event sources, which just need the factories to hold data.
//The way this is done is by using recursion on the variadic template arguments.

/**************************************************************** TYPE DECLARATIONS ****************************************************************/

//Default class (used when DataTypes size = 0)
template <typename... DataTypes>
class JSourceFactoryGenerator;

//Specialization (used when DataTypes size >= 1)
template <typename HeadType, typename... TailTypes>
class JSourceFactoryGenerator<HeadType, TailTypes...>;

/**************************************************************** DEFAULT TYPE DEFINITION ****************************************************************/

//Default class (used when DataTypes size = 0)
template <typename... DataTypes>
class JSourceFactoryGenerator : public JFactoryGenerator
{
	public:
		void GenerateFactories(JFactorySet *factory_set);

		template <typename ContainerType>
		static void MakeFactories(std::back_insert_iterator<ContainerType> aIterator);
};

/************************************************************* TYPE SPECIALIZATION DEFINITION *************************************************************/

//Specialization (used when DataTypes size >= 1)
template <typename HeadType, typename... TailTypes>
struct JSourceFactoryGenerator<HeadType, TailTypes...> : public JFactoryGenerator
{
	public:
		void GenerateFactories(JFactorySet *factory_set);

		template <typename ContainerType>
		static void MakeFactories(std::back_insert_iterator<ContainerType> aIterator);
};

/*********************************************************** MEMBER FUNCTION DEFINITIONS: DEFAULT ***********************************************************/

template <typename... DataTypes>
void JSourceFactoryGenerator<DataTypes...>::GenerateFactories(JFactorySet *factory_set)
{
	//Do nothing: This is only called when DataTypes is empty.
	//If DataTypes is not empty, template deduction extracts HeadType and thus uses the JSourceFactoryGenerator specialization.
	return;
}

template <typename... DataTypes>
template <typename ContainerType>
void JSourceFactoryGenerator<DataTypes...>::MakeFactories(std::back_insert_iterator<ContainerType> aIterator)
{
	//Do nothing: This is only called when DataTypes is empty.
	//If DataTypes is not empty, template deduction extracts HeadType and thus uses the JSourceFactoryGenerator specialization.
}

/******************************************************** MEMBER FUNCTION DEFINITIONS: SPECIALIZATION ********************************************************/

template <typename HeadType, typename... TailTypes>
void JSourceFactoryGenerator<HeadType, TailTypes...>::GenerateFactories(JFactorySet *factory_set)
{
	//Initialize vector
	std::vector<JFactory*> sFactories;
	sFactories.reserve(sizeof...(TailTypes) + 1);

	//Make factories and return
	MakeFactories(std::back_inserter(sFactories));
	for(auto fac : sFactories) factory_set->Add(fac);
}

template <typename HeadType, typename... TailTypes>
template <typename ContainerType>
void JSourceFactoryGenerator<HeadType, TailTypes...>::MakeFactories(std::back_insert_iterator<ContainerType> aIterator)
{
	//Create the factory for the head type
	aIterator = static_cast<JFactory*>(new JFactoryT<HeadType>);
	//Continue making factories for tail types (unless tail is empty)
	JSourceFactoryGenerator<TailTypes...>::MakeFactories(aIterator);
}

#endif // _JSourceFactoryGenerator_h_
