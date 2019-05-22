//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Authors: Nathan Brei, David Lawrence, Edward Brash
//

#include <iostream>
#include <JANA/JApplication.h>
#include <JANA/JVersion.h>
#include <JANA/JSignalHandler.h>

#include "JBenchmarker.h"


void PrintUsage() {
	/// Prints jana.cc command-line options to stdout, for use by the CLI.
	/// This does not include JANA parameters, which come from
	/// JParameterManager::PrintParameters() instead.

	std::cout << std::endl;
	std::cout << "Usage:" << std::endl;
	std::cout << "    jana [options] source1 source2 ..." << std::endl << std::endl;

	std::cout << "Description:" << std::endl;
	std::cout << "    Command-line interface for running JANA plugins. This can be used to" << std::endl;
	std::cout << "    read in events and process them. Command-line flags control configuration" << std::endl;
	std::cout << "    while additional arguments denote input files, which are to be loaded and" << std::endl;
	std::cout << "    processed by the appropriate EventSource plugin." << std::endl << std::endl;

	std::cout << "Options:" << std::endl;
	std::cout << "   -h   --help                  Display this message" << std::endl;
	std::cout << "   -v   --version               Display version information" << std::endl;
	std::cout << "   -c   --configs               Display configuration parameters" << std::endl;
	std::cout << "   -l   --loadconfigs <file>    Load configuration parameters from file" << std::endl;
	std::cout << "   -d   --dumpconfigs <file>    Dump configuration parameters to file" << std::endl;
	std::cout << "   -b   --benchmark             Run in benchmark mode" << std::endl;
	std::cout << "   -Pkey=value                  Specify a configuration parameter" << std::endl << std::endl;

	std::cout << "Example:" << std::endl;
	std::cout << "    jana -Pplugins=plugin1,plugin2,plugin3 -Pnthreads=8 inputfile1.txt" << std::endl << std::endl;

}


void PrintVersion() {
	/// Prints JANA version information to stdout, for use by the CLI.

	std::cout << "          JANA version: " << JVersion::GetVersion()  << std::endl;
	std::cout << "        JANA ID string: " << JVersion::GetIDstring() << std::endl;
	std::cout << "     JANA git revision: " << JVersion::GetRevision() << std::endl;
	std::cout << "JANA last changed date: " << JVersion::GetDate()     << std::endl;
	std::cout << "           JANA source: " << JVersion::GetSource()   << std::endl;
}


enum Flag {Unknown, ShowUsage, ShowVersion, ShowConfigs, LoadConfigs, DumpConfigs, Benchmark};

struct UserOptions {
	/// Code representation of all user options.
	/// This lets us cleanly separate args parsing from execution.

	std::map<Flag, bool> flags;
	JParameterManager params;
	std::vector<std::string> eventSources;
	std::string load_config_file;
	std::string dump_config_file;
};


int Execute(UserOptions& options) {

	int exitStatus = 0;

	if (options.flags[ShowUsage]) {
		// Show usage information and exit immediately
		PrintUsage();
		exitStatus = -1;
	}
	else if (options.flags[ShowVersion]) {
		// Show version information and exit immediately
		PrintVersion();
	}
	else {  // All modes which require a JApplication

	    std::cout << "JANA " << JVersion::GetVersion() << " [" << JVersion::GetRevision() << "]" << std::endl;
		if (options.flags[LoadConfigs]) {
			// If the user specified an external config file, we should definitely use that
			try {
				options.params.ReadConfigFile(options.load_config_file);
			}
			catch (JException& e) {
				std::cout << "Problem loading config file '" << options.load_config_file << "'. Exiting." << std::endl << std::endl;
				exit(-1);
			}
			std::cout << "Loaded config file '" << options.load_config_file << "'." << std::endl << std::endl;
		}

		auto params_copy = new JParameterManager(options.params); // JApplication owns params_copy, does not own eventSources
		japp = new JApplication(params_copy);
		for (auto event_src : options.eventSources) {
			japp->Add(event_src);
		}
		AddSignalHandlers();

		if (options.flags[ShowConfigs]) {
			// Load all plugins, collect all parameters, exit without running anything
			japp->Initialize();
			if (options.flags[Benchmark]) {
				JBenchmarker benchmarker(japp);  // Show benchmarking configs only if benchmarking mode specified
			}
			japp->GetJParameterManager()->PrintParameters(true);
		}
		else if (options.flags[DumpConfigs]) {
			// Load all plugins, dump parameters to file, exit without running anything
			japp->Initialize();
			std::cout << std::endl << "Writing configuration options to file: " << options.dump_config_file << std::endl;
			japp->GetJParameterManager()->WriteConfigFile(options.dump_config_file);
		}
		else if (options.flags[Benchmark]) {
			// Run JANA in benchmark mode
			JBenchmarker benchmarker(japp); // Benchmarking params override default params
			benchmarker.RunUntilFinished(); // Benchmarker will control JApp Run/Stop
		}
		else {
			// Run JANA in normal mode
			japp->Run();
		}
		exitStatus = japp->GetExitCode();
		delete japp;
	}
	exit(exitStatus);
}


int main(int nargs, char *argv[]) {

	UserOptions options;

	std::map<std::string, Flag> tokenizer;
	tokenizer["-h"] = ShowUsage;
	tokenizer["--help"] = ShowUsage;
	tokenizer["-v"] = ShowVersion;
	tokenizer["--version"] = ShowVersion;
	tokenizer["-c"] = ShowConfigs;
	tokenizer["--configs"] = ShowConfigs;
	tokenizer["-l"] = LoadConfigs;
	tokenizer["--loadconfigs"] = LoadConfigs;
	tokenizer["-d"] = DumpConfigs;
	tokenizer["--dumpconfigs"] = DumpConfigs;
	tokenizer["-b"] = Benchmark;
	tokenizer["--benchmark"] = Benchmark;

	if (nargs==1) {
		options.flags[ShowUsage] = true;
	}

	for (int i=1; i<nargs; i++){

		string arg = argv[i];
		//std::cout << "Found arg " << arg << std::endl;

		if(argv[i][0] != '-') {
			options.eventSources.push_back(arg);
			continue;
		}

		switch (tokenizer[arg]) {

			case Benchmark:
				options.flags[Benchmark] = true;
				break;

			case ShowUsage:
				options.flags[ShowUsage] = true;
				break;

			case ShowVersion:
				options.flags[ShowVersion] = true;
				break;

			case ShowConfigs:
				options.flags[ShowConfigs] = true;
				break;

			case LoadConfigs:
				options.flags[LoadConfigs] = true;
				if (i+1 < nargs && argv[i+1][0] != '-') {
					options.load_config_file = argv[i + 1];
					i += 1;
				}
				else {
					options.load_config_file = "jana.config";
				}
				break;

			case DumpConfigs:
				options.flags[DumpConfigs] = true;
				if (i+1 < nargs && argv[i+1][0] != '-') {
					options.dump_config_file = argv[i + 1];
					i += 1;
				}
				else {
					options.dump_config_file = "jana.config";
				}
				break;

			case Unknown:
				if (argv[i][0] == '-' && argv[i][1] == 'P') {

					size_t pos = arg.find("=");
					if ((pos != string::npos) && (pos > 2)) {
						string key = arg.substr(2, pos - 2);
						string val = arg.substr(pos + 1);
						options.params.SetParameter(key, val);
					} else {
						std::cout << "Invalid JANA parameter '" << arg
								  << "': Expected format -Pkey=value" << std::endl;
						options.flags[ShowConfigs] = true;
					}
				} else {
					std::cout << "Invalid command line flag '" << arg << "'" << std::endl;
					options.flags[ShowUsage] = true;
				}
		}
	}
	Execute(options);
}




