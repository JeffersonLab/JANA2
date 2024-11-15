
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <csignal>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <JANA/JApplication.h>

/// JSignalHandler bundles together the logic for querying a JApplication
/// about its JStatus with signal handlers for USR1, USR2, and CTRL-C.
namespace JSignalHandler {

extern JApplication* g_app;
extern int g_sigint_count;
extern JLogger* g_logger;
extern std::string g_path_to_named_pipe;
extern std::map<pthread_t, std::string> g_thread_reports;
extern std::atomic_int g_thread_report_count;


void create_named_pipe(const std::string& path_to_named_pipe);
void send_to_named_pipe(const std::string& path_to_named_pipe, const std::string& data);
std::string produce_overall_report();
void send_overall_report_to_named_pipe();
void handle_sigint(int);
void handle_usr1(int);
void handle_usr2(int);
void handle_sigsegv(int /*signal_number*/, siginfo_t* /*signal_info*/, void* /*context*/);
void register_handlers(JApplication* app);

}; // namespace JSignalHandler

