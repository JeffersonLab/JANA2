
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

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
