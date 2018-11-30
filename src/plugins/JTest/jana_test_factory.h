// $Id$
//
//    File: jana_test_factory.h
// Created: Mon Oct 23 22:39:14 EDT 2017
// Creator: davidl (on Darwin harriet 15.6.0 i386)
//

#ifndef _jana_test_factory_
#define _jana_test_factory_

#include <JANA/JFactoryT.h>
#include <JANA/JEvent.h>
#include "jana_test.h"

#include <random>

class jana_test_factory : public JFactoryT<jana_test>
{
	public:
		//STRUCTORS
		jana_test_factory(void);
		~jana_test_factory(void);

		virtual void Init(void);
		virtual void ChangeRun(const std::shared_ptr<const JEvent>& aEvent);
		virtual void Process(const std::shared_ptr<const JEvent>& aEvent);

	private:

		std::mt19937 mRandomGenerator;
};

#endif // _jana_test_factory_

