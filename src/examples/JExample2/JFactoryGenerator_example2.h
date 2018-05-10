
#ifndef _JFACTORYGENERATOR_EXAMPLE2_H_
#define _JFACTORYGENERATOR_EXAMPLE2_H_

#include <JANA/JFactoryGenerator.h>
#include <JANA/JFactoryT.h>
#include "MyCluster.h"
#include "MyHit.h"

class JFactoryGenerator_example2:public JFactoryGenerator{
	public:

	void GenerateFactories(JFactorySet *factory_set){
		
		factory_set->Add( new JFactoryT<MyHit>()     );
		factory_set->Add( new JFactoryT<MyCluster>() );

	}
};

#endif   // _JFACTORYGENERATOR_EXAMPLE2_H_
