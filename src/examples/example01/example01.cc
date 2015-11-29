

#include <iostream>
using namespace std;

#include <JANA/JApplication.h>
using namespace jana;

#include "JEventSourceGenerator_MySource.h"


int main(int narg, char *argv[])
{
	JApplication *japp = new JApplication(narg, argv);

	japp->AddEventSourceGenerator(new JEventSourceGenerator_MySource());

	japp->Run();

	int exit_code = japp->GetExitCode();
	
	delete japp;

	return exit_code;
}

