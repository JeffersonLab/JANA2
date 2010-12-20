

#include <iostream>
using namespace std;

#include <JANA/JApplication.h>
using namespace jana;

#include "JEventSourceGenerator_MySource.h"
#include "JFactoryGenerator_RawHit.h"
#include "JEventProcessor_MyProcessor.h"


int main(int narg, char *argv[])
{
	JApplication *japp = new JApplication(narg, argv);

	japp->AddEventSourceGenerator(new JEventSourceGenerator_MySource);
	japp->AddFactoryGenerator(new JFactoryGenerator_RawHit);
	japp->AddProcessor(new JEventProcessor_MyProcessor);

	japp->Run();

	return 0;
}

