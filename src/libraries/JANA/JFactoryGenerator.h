
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <vector>

#include <JANA/JFactorySet.h>

#ifndef _JFactoryGenerator_h_
#define _JFactoryGenerator_h_

class JFactoryGenerator{
	public:
		virtual ~JFactoryGenerator() {};
		
		virtual void GenerateFactories(JFactorySet *factory_set) = 0;
};

template <class T>
class JFactoryGeneratorT : public JFactoryGenerator {
public:

	~JFactoryGeneratorT() {};

	void GenerateFactories(JFactorySet *factory_set) override {
		factory_set->Add(new T {});
	}
};


#endif // _JFactoryGenerator_h_


