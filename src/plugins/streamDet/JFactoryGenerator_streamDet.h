
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
