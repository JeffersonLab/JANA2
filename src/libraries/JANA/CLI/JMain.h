
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#ifndef JANA2_JMAIN_H
#define JANA2_JMAIN_H

#include <JANA/JApplication.h>

namespace jana {

enum Flag {Unknown, ShowUsage, ShowVersion, ShowConfigs, LoadConfigs, DumpConfigs, Benchmark};

struct UserOptions {
	/// Code representation of all user options.
	/// This lets us cleanly separate args parsing from execution.

	std::map<Flag, bool> flags;
	std::map<std::string, std::string> params;
	std::vector<std::string> eventSources;
	std::string load_config_file;
	std::string dump_config_file;
};

void PrintUsage();
void PrintUsageOptions();
void PrintVersion();
UserOptions ParseCommandLineOptions(int nargs, char *argv[], bool expect_extra=true);
JApplication* CreateJApplication(UserOptions& options);
int Execute(JApplication* app, UserOptions& options);

}

#endif //JANA2_JMAIN_H
