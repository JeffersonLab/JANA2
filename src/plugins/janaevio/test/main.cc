

#include <JANA/JApplication.h>
//#include <JEventSourceEVIOGenerator.h>

int main(int narg, char *argv[])
{
	JApplication japp(narg, argv);
	
	//japp.AddEventSourceGenerator(new JEventSourceEVIOGenerator());

	japp.Run(NULL,1);
	
	return 0;
}

