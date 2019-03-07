#ifndef JANA_JSIGNALHANDLER_H_
#define JANA_JSIGNALHANDLER_H_

#include <signal.h>
#include <thread>

#include <JANA/JApplication.h>
#include <JANA/JStatus.h>

int gSIGINT_RECEIVED = 0;

//-----------------------------------------------------------------
// ctrlCHandle
//-----------------------------------------------------------------
void ctrlCHandle(int x)
{
	gSIGINT_RECEIVED++;

	if (japp == nullptr) {
		exit(-2);
	}

	if (gSIGINT_RECEIVED == 1) {
		jout << "Exiting gracefully...\n";
		japp->Quit(false);
	} 
	else if (gSIGINT_RECEIVED == 2) {
		jout << "Exiting without waiting for threads to join...\n";
		japp->Quit(true);
	}
	else {
		jout << "Exiting immediately.\n";
		exit(-2);
	}
}

//-----------------------------------------------------------------
// USR1Handle
//-----------------------------------------------------------------
void USR1Handle(int x)
{
	thread th( JStatus::Report );
	th.detach();
}

//-----------------------------------------------------------------
// USR2Handle
//-----------------------------------------------------------------
void USR2Handle(int x)
{
	JStatus::RecordBackTrace();
}

//-----------------------------------------------------------------
// SIGSEGVHandle
//-----------------------------------------------------------------
void SIGSEGVHandle(int aSignalNumber, siginfo_t* aSignalInfo, void* aContext)
{
	JStatus::Report();
}

//-----------------------------------------------------------------
// AddSignalHandlers
//-----------------------------------------------------------------
void AddSignalHandlers(void)
{
	/// Add special handles for system signals. The handlers will catch SIGINT
	/// signals that the user may send (e.g. by hitting ctl-C) to indicate
	/// they want data processing to stop and the program to end. When a SIGINT
	/// is received, JANA will try and shutdown the program cleanly, allowing
	/// the processing threads to finish up the events they are working on.
	/// The first 2 SIGINT signals received will tell JANA to shutdown gracefully.
	/// On the 3rd SIGINT, the program will try to exit immediately.


	//Define signal action
	struct sigaction sSignalAction;
	sSignalAction.sa_sigaction = SIGSEGVHandle;
	sSignalAction.sa_flags = SA_RESTART | SA_SIGINFO;

	//Clear and set signals
	sigemptyset(&sSignalAction.sa_mask);
	sigaction(SIGSEGV, &sSignalAction, nullptr);

	// Set up to catch SIGINTs for graceful exits
	signal(SIGINT,ctrlCHandle);

	// Set up to catch USR1's and USR2's for status reporting
	signal(SIGUSR1,USR1Handle);
	signal(SIGUSR2,USR2Handle);
}


#endif
