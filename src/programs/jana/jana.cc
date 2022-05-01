
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include <JANA/CLI/JMain.h>

int main(int argc, char* argv[]) {

	auto options = jana::ParseCommandLineOptions(argc, argv, false);

	if (options.flags[jana::ShowUsage]) {
		// Show usage information and exit immediately
		jana::PrintUsage();
		return -1;
	}
	if (options.flags[jana::ShowVersion]) {
		// Show version information and exit immediately
		jana::PrintVersion();
		return -1;
	}
	auto app = jana::CreateJApplication(options);

	// Keep japp global around for backwards compatibility. Don't use this in new code, however
	japp = app;

	auto exit_code = jana::Execute(app, options);
	delete app;
	return exit_code;
}

