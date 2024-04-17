
// Copyright 2020, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.


#include <JANA/Services/JLoggingService.h>

#include <iostream>


template <>
inline void JParameterManager::Parse(const std::string& in, JLogger::Level& out) {
    std::string token(in);
    std::transform(in.begin(), in.end(), token.begin(), ::tolower);
    if (std::strcmp(token.c_str(), "trace") == 0) { 
        out = JLogger::Level::TRACE;
    }
    else if (std::strcmp(token.c_str(), "debug") == 0) { 
        out = JLogger::Level::DEBUG;
    }
    else if (std::strcmp(token.c_str(), "info") == 0) { 
        out = JLogger::Level::INFO;
    }
    else if (std::strcmp(token.c_str(), "warn") == 0) { 
        out = JLogger::Level::WARN;
    }
    else if (std::strcmp(token.c_str(), "error") == 0) { 
        out = JLogger::Level::ERROR;
    }
    else if (std::strcmp(token.c_str(), "fatal") == 0) { 
        out = JLogger::Level::FATAL;
    }
    else if (std::strcmp(token.c_str(), "off") == 0) { 
        out = JLogger::Level::OFF;
    }
    else {
        throw JException("Unable to parse log level: '%s'. Options are: TRACE, DEBUG, INFO, WARN, ERROR, FATAL, OFF", in.c_str());
    }
}

void JLoggingService::acquire_services(JServiceLocator* serviceLocator) {

    auto params = serviceLocator->get<JParameterManager>();
    params->SetDefaultParameter("log:global", m_global_log_level, "Default log level");

    std::vector<std::string> groups;
    params->SetDefaultParameter("log:off", groups, "Comma-separated list of loggers that should be turned off completely");
    for (auto& s : groups) {
        m_local_log_levels[s] = JLogger::Level::OFF;
    }
    groups.clear();
    params->SetDefaultParameter("log:fatal", groups, "Comma-separated list of loggers that should only print FATAL");
    for (auto& s : groups) {
        m_local_log_levels[s] = JLogger::Level::FATAL;
    }
    groups.clear();
    params->SetDefaultParameter("log:error", groups, "Comma-separated list of loggers that should only print ERROR or higher");
    for (auto& s : groups) {
        m_local_log_levels[s] = JLogger::Level::ERROR;
    }
    groups.clear();
    params->SetDefaultParameter("log:warn", groups, "Comma-separated list of loggers that should only print WARN or higher");
    for (auto& s : groups) {
        m_local_log_levels[s] = JLogger::Level::WARN;
    }
    groups.clear();
    params->SetDefaultParameter("log:info", groups, "Comma-separated list of loggers that should only print INFO or higher");
    for (auto& s : groups) {
        m_local_log_levels[s] = JLogger::Level::INFO;
    }
    groups.clear();
    params->SetDefaultParameter("log:debug", groups, "Comma-separated list of loggers that should only print DEBUG or higher");
    for (auto& s : groups) {
        m_local_log_levels[s] = JLogger::Level::DEBUG;
    }
    groups.clear();
    params->SetDefaultParameter("log:trace", groups, "Comma-separated list of loggers that should print everything");
    for (auto& s : groups) {
        m_local_log_levels[s] = JLogger::Level::TRACE;
    }
    // Set the log level on the parameter manager, resolving the chicken-and-egg problem.
    params->SetLogger(get_logger("JParameterManager"));
}


void JLoggingService::set_level(JLogger::Level level) { 
    m_global_log_level = level; 
}


void JLoggingService::set_level(std::string className, JLogger::Level level) {
    m_local_log_levels[className] = level;
}


JLogger JLoggingService::get_logger() {
    return JLogger(m_global_log_level);
}


JLogger JLoggingService::get_logger(std::string className) {

    JLogger logger;
    logger.show_classname = true;
    logger.className = className;

    auto search = m_local_log_levels.find(className);
    if (search != m_local_log_levels.end()) {
        logger.level = search->second;
    } else {
        logger.level = m_global_log_level;
    }
    return logger;
}




