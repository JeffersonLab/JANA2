
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JSignalHandler.h"

#include <csignal>
#include <thread>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include <JANA/JApplication.h>

/// JSignalHandler bundles together the logic for querying a JApplication
/// about its JStatus with signal handlers for USR1, USR2, and CTRL-C.
namespace JSignalHandler {

JApplication* g_app;
int g_sigint_count = 0;
JLogger* g_logger;
std::string g_path_to_named_pipe = "/tmp/jana_status";
std::map<pthread_t, std::string> g_thread_reports;
std::atomic_int g_thread_report_count;


void create_named_pipe(const std::string& path_to_named_pipe) {

    LOG_WARN(*g_logger) << "Creating pipe named \"" << g_path_to_named_pipe
                        << "\" for status info." << LOG_END;

    mkfifo(path_to_named_pipe.c_str(), 0666);
}


void send_to_named_pipe(const std::string& path_to_named_pipe, const std::string& data) {

    int fd = open(path_to_named_pipe.c_str(), O_WRONLY);
    if (fd >=0) {
        write(fd, data.c_str(), data.length()+1);
        close(fd);
    }
    else {
        LOG_ERROR(*g_logger) << "Unable to open named pipe '" << g_path_to_named_pipe << "' for writing. \n"
        << "  You can use a different named pipe for status info by setting the parameter `jana:status_fname`.\n"
        << "  The status report will still show up in the log." << LOG_END;
    }
}

void produce_thread_report() {
    std::stringstream bt_str;
    make_backtrace(bt_str);
    g_thread_reports[pthread_self()] = bt_str.str();
}

/// If something goes wrong, we want to signal all threads to assemble a report
/// Whereas USR1 is meant to be triggered externally and is caught by one thread,
/// produce_overall_report triggers USR2 and is caught by all threads.
std::string produce_overall_report() {
    std::stringstream ss;

    // Include detailed report from JApplication
    auto t = time(nullptr);
    ss << "JANA status report: " << ctime(&t) << std::endl;

    // Include backtraces from each individual thread
    if( typeid(std::thread::native_handle_type) == typeid(pthread_t) ){
        ss << "Thread model: pthreads" << std::endl;

        // Send every worker thread (but not self) the USR2 signal
        auto main_thread_id = pthread_self();
        std::vector<pthread_t> threads; // TODO: Populate this
        g_thread_report_count = threads.size();
        for (auto& thread_id : threads) {
            if (main_thread_id == thread_id) {
                pthread_kill(thread_id, SIGUSR2);
            }
        }

        // Assemble backtrace for own thread
        std::stringstream bt_str;
        make_backtrace(bt_str);
        g_thread_reports[main_thread_id] = bt_str.str();

        // Wait for all other threads to finish handling USR2
        for(int i=0; i<1000; i++){
            std::this_thread::sleep_for(std::chrono::microseconds(1000));
            if (g_thread_report_count == 0) break;
        }

        // Assemble overall backtrace
        for (const auto& thread_report : g_thread_reports) {
            ss << thread_report.first << ": " << std::endl << thread_report.second << std::endl;
        }

        // Clear backtrace
        g_thread_reports.clear();
        // TODO: Backtrace memory use is unsafe
    }
    else {
        ss << "Thread model: unknown" << std::endl;
    }
    return ss.str();
}

void send_overall_report_to_named_pipe() {
    LOG_WARN(*g_logger) << "Caught USR1 signal! Sending status report to named pipe. `cat " << g_path_to_named_pipe << "` to view." << LOG_END;
    send_to_named_pipe(g_path_to_named_pipe, produce_overall_report());
}


/// Handle SIGINT signals (e.g. from hitting Ctrl-C). When a SIGINT
/// is received, JANA will try and shutdown the program cleanly, allowing
/// the processing threads to finish up the events they are working on.
/// The first 2 SIGINT signals received will tell JANA to shutdown gracefully.
/// On the 3rd SIGINT, the program will try to exit immediately.
void handle_sigint(int) {
    g_app->HandleSigint();
}

void handle_usr1(int) {
    std::thread th(send_overall_report_to_named_pipe);
    th.detach();
}

void handle_usr2(int) {
    produce_thread_report();
}

void handle_sigsegv(int /*signal_number*/, siginfo_t* /*signal_info*/, void* /*context*/) {
    LOG_FATAL(*g_logger) << "Segfault detected! Printing backtraces and exiting." << LOG_END;
    auto report = produce_overall_report();
    LOG_FATAL(*g_logger) << report << LOG_END;
    exit(static_cast<int>(JApplication::ExitCode::Segfault));
}


/// Add special handles for system signals.
void register_handlers(JApplication* app) {
    assert (app != nullptr);
    g_app = app;
    g_logger = &default_cout_logger;
    *g_logger = app->GetJParameterManager()->GetLogger("jana");
    // Note that this updates the static default_cout_logger to match the user-provided jana:loglevel.
    // It would be nice to do this in a less unexpected place, and hopefully that will naturally
    // emerge from future refactorings.

    g_app->GetJParameterManager()->SetDefaultParameter("jana:status_fname", g_path_to_named_pipe,
        "Filename of named pipe for retrieving instantaneous status info");
    create_named_pipe(g_path_to_named_pipe);

    //Define signal action
    struct sigaction sSignalAction;
    sSignalAction.sa_sigaction = handle_sigsegv;
    sSignalAction.sa_flags = SA_RESTART | SA_SIGINFO;

    //Clear and set signals
    sigemptyset(&sSignalAction.sa_mask);
    sigaction(SIGSEGV, &sSignalAction, nullptr);

    LOG_WARN(*g_logger) << "Setting signal handler USR1. Use to write status info to the named pipe." << LOG_END;
    signal(SIGUSR1, handle_usr1);
    signal(SIGUSR2, handle_usr2);
    LOG_WARN(*g_logger) << "Setting signal handler SIGINT (Ctrl-C). Use a single SIGINT to enter the Inspector, or multiple SIGINTs for an immediate shutdown." << LOG_END;
    signal(SIGINT,  handle_sigint);
}


}; // namespace JSignalHandler

