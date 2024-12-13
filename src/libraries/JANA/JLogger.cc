
#include <JANA/JLogger.h>

JLogger jout {JLogger::Level::INFO, &std::cout, "jana"};
JLogger jerr {JLogger::Level::ERROR, &std::cerr, "jana"};


