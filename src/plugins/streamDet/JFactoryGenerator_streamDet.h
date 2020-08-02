
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#ifndef _JFactoryGenerator_streamDet_h_
#define _JFactoryGenerator_streamDet_h_

#include <JANA/JFactoryGenerator.h>
#include <JANA/JFactoryT.h>

#include "ADCSample.h"

class JFactoryGenerator_streamDet: public JFactoryGenerator{

 public:

  void GenerateFactories(JFactorySet *factory_set){
		
    factory_set->Add(new JFactoryT<ADCSample>());

  }

};

#endif // _JFactoryGenerator_streamDet_h_
