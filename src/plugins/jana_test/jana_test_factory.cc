// $Id$
//
//    File: jana_test_factory.cc
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#include "jana_test_factory.h"

#include <chrono>
#include <iostream>

//------------------
// Constructor
//------------------
jana_test_factory::jana_test_factory(void) : JFactory<jana_test>("jana_test_factory")
{
	//This is the new "init()" function

	//Seed random number generator //not ideal!
	auto sTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	mRandomGenerator.seed(sTime);
}

//------------------
// Destructor
//------------------
jana_test_factory::~jana_test_factory(void)
{
	//This is the new "fini()" function
}

//------------------
// ChangeRun
//------------------
void jana_test_factory::ChangeRun(const std::shared_ptr<const JEvent>& aEvent)
{

}

//------------------
// Create
//------------------
void jana_test_factory::Create(const std::shared_ptr<const JEvent>& aEvent)
{
	// Code to generate factory data goes here. Add it like:
	//
	// mData.emplace_back(/* jana_test constructor arguments */);
	//
	// Note that the objects you create here will be deleted later
	// by the system and the mData vector will be cleared automatically.

	//Generate a random # of objects
//	auto sNumObjectsDistribution = std::uniform_int_distribution<std::size_t>(1, 20);
	auto sNumObjectsDistribution = std::uniform_int_distribution<std::size_t>(1, 20);
	auto sNumObjects = sNumObjectsDistribution(mRandomGenerator);
	for(std::size_t si = 0; si < sNumObjects; si++)
	{
		//Emplace new object onto the back of the data vector
		mData.emplace_back(mRandomGenerator(), si); //Random energy, id = object#
		auto& sObject = mData.back(); //Get reference to new object

		//Supply busy work to take time: Generate a bunch of randoms
//		auto sNumRandomsDistribution = std::uniform_int_distribution<std::size_t>(1000, 2000);
		auto sNumRandomsDistribution = std::uniform_int_distribution<std::size_t>(1000, 2000);
		auto sNumRandoms = sNumRandomsDistribution(mRandomGenerator);
		for(std::size_t sj = 0; sj < sNumRandoms; sj++)
			sObject.AddRandom(mRandomGenerator());
	}
}
