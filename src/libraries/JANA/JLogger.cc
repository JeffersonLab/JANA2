
#include <JANA/JLogger.h>

JLogger jout {JLogger::Level::INFO, &std::cout, "jana"};
JLogger jerr {JLogger::Level::ERROR, &std::cerr, "jana"};

thread_local int JLogger::thread_id = -1;
std::atomic_int JLogger::next_thread_id = 0;

