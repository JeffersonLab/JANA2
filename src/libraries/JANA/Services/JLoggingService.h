
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <JANA/JLogger.h>
#include <JANA/Services/JParameterManager.h>
#include <JANA/Services/JServiceLocator.h>


// Convenience macros for temporary debugging statements.
#ifndef _DBG__
#define _DBG__ std::cerr<<__FILE__<<":"<<__LINE__<<std::endl
#define _DBG_  std::cerr<<__FILE__<<":"<<__LINE__<<" "
#endif // _DBG__


// Helper function for truncating long strings to keep our log readable
inline std::string ltrunc(std::string original, size_t desired_length) {
    auto n = original.length();
    if (n <= desired_length) return original;
    return "\u2026" + original.substr(n - desired_length, desired_length - 1);
}


class JLoggingService : public JService {

    JLogger::Level m_global_log_level = JLogger::Level::INFO;

    std::map<std::string, JLogger::Level> m_local_log_levels;

public:

    void acquire_services(JServiceLocator* serviceLocator) override;

    void set_level(JLogger::Level level);

    void set_level(std::string className, JLogger::Level level);

    JLogger get_logger();

    JLogger get_logger(std::string className);

};



