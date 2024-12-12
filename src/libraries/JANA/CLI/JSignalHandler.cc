
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#include "JSignalHandler.h"

#include <JANA/JApplication.h>
#include <JANA/Engine/JExecutionEngine.h>
#include <JANA/Utils/JBacktrace.h>

#include <unistd.h>
#include <csignal>

/// JSignalHandler bundles together the logic for querying a JApplication
/// about its JStatus with signal handlers for USR1, USR2, and CTRL-C.
namespace JSignalHandler {

JApplication* g_app;
JLogger* g_logger;


/// Handle SIGINT signals (e.g. from hitting Ctrl-C). When a SIGINT
/// is received, JANA will try and shutdown the program cleanly, allowing
/// the processing threads to finish up the events they are working on.
/// The first 2 SIGINT signals received will tell JANA to shutdown gracefully.
/// On the 3rd SIGINT, the program will try to exit immediately.
void handle_sigint(int) {
    if (g_app->IsInitialized()) {
        g_app->GetService<JExecutionEngine>()->HandleSIGINT();
    }
    else {
        exit(-2);
    }
}

void handle_usr1(int) {
    if (g_app->IsInitialized()) {
        g_app->GetService<JExecutionEngine>()->HandleSIGUSR1();
    }
}

void handle_usr2(int) {
    if (g_app->IsInitialized()) {
        g_app->GetService<JExecutionEngine>()->HandleSIGUSR2();
    }
}

void handle_tstp(int) {
    if (g_app->IsInitialized()) {
        g_app->GetService<JExecutionEngine>()->HandleSIGTSTP();
    }
}

void handle_sigsegv(int /*signal_number*/, siginfo_t* /*signal_info*/, void* /*context*/) {
    LOG_FATAL(*g_logger) << "Segfault detected!" << LOG_END;
    JBacktrace backtrace;
    backtrace.Capture(3);
    LOG_FATAL(*g_logger) << "Hard exit due to segmentation fault! Backtrace:\n\n" << backtrace.ToString() << LOG_END;
    _exit(static_cast<int>(JApplication::ExitCode::Segfault)); 
    // _exit() is async-signal-safe, whereas exit() is not
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

    // We capture a dummy backtrace to warm it up before it gets called inside a signal handler.
    // Because backtrace() dynamically loads libgcc, calling malloc() in the process, it is not
    // async-signal-safe until this warmup has happened. Thus we prevent a rare deadlock and several
    // TSAN and Helgrind warnings.
    JBacktrace backtrace;
    backtrace.Capture();

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
    signal(SIGTSTP, handle_tstp);
    LOG_WARN(*g_logger) << "Setting signal handler SIGINT (Ctrl-C). Use a single SIGINT to enter the Inspector, or multiple SIGINTs for an immediate shutdown." << LOG_END;
    signal(SIGINT,  handle_sigint);
}


}; // namespace JSignalHandler

