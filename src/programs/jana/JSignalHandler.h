#ifndef JANA_JSIGNALHANDLER_H_
#define JANA_JSIGNALHANDLER_H_

#include <csignal>
#include <thread>

#include <JANA/JApplication.h>
#include <JANA/Status/JStatus.h>

/// JSignalHandler bundles together the logic for querying a JApplication
/// about its JStatus with signal handlers for USR1, USR2, and CTRL-C.
namespace JSignalHandler {

JApplication* g_app;
int g_sigint_count = 0;
JLogger* g_logger;

/// Handle SIGINT signals (e.g. from hitting Ctrl-C). When a SIGINT
/// is received, JANA will try and shutdown the program cleanly, allowing
/// the processing threads to finish up the events they are working on.
/// The first 2 SIGINT signals received will tell JANA to shutdown gracefully.
/// On the 3rd SIGINT, the program will try to exit immediately.
void handle_sigint(int) {
    g_sigint_count++;
    switch (g_sigint_count) {
        case 1:
            LOG_FATAL(*g_logger) << "\nExiting gracefully..." << LOG_END;
            g_app->Quit(false);
            break;
        case 2:
            LOG_FATAL(*g_logger) << "\nExiting without waiting for threads to join..." << LOG_END;
            japp->Quit(true);
            break;
        default:
            LOG_FATAL(*g_logger) << "\nExiting immediately." << LOG_END;
            exit(-2);
    }
}

void handle_usr1(int) {
    std::thread th(JStatus::Report );
    th.detach();
}

void handle_usr2(int) {
    JStatus::RecordBackTrace();
}

void handle_sigsegv(int signal_number, siginfo_t* signal_info, void* context) {
    JStatus::Report();
}

/// Add special handles for system signals.
void register_handlers(JApplication* app) {
    assert (app != nullptr);
    g_app = app;
    g_logger = &default_cout_logger;

    //Define signal action
    struct sigaction sSignalAction;
    sSignalAction.sa_sigaction = handle_sigsegv;
    sSignalAction.sa_flags = SA_RESTART | SA_SIGINFO;

    //Clear and set signals
    sigemptyset(&sSignalAction.sa_mask);
    sigaction(SIGSEGV, &sSignalAction, nullptr);

    signal(SIGINT,  handle_sigint);
    signal(SIGUSR1, handle_usr1);
    signal(SIGUSR2, handle_usr2);
}


}; // namespace JSignalHandler

#endif
