// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JException.h>
#include <ostream>
#include <sstream>

enum class JEventLevel { Run, Subrun, SlowControls, Timeslice, Block, PhysicsEvent, Subevent, Task, None };

inline std::ostream& operator<<(std::ostream& os, JEventLevel level) {
    switch (level) {
        case JEventLevel::Run: os << "Run"; break;
        case JEventLevel::Subrun: os << "Subrun"; break;
        case JEventLevel::Timeslice: os << "Timeslice"; break;
        case JEventLevel::Block: os << "Block"; break;
        case JEventLevel::SlowControls: os << "SlowControls"; break;
        case JEventLevel::PhysicsEvent: os << "PhysicsEvent"; break;
        case JEventLevel::Subevent: os << "Subevent"; break;
        case JEventLevel::Task: os << "Task"; break;
        default: os << "None"; break;
    }
    return os;
}

inline std::string toString(JEventLevel level) {
    std::stringstream ss;
    ss << level;
    return ss.str();
}

inline JEventLevel parseEventLevel(const std::string& level) {
    if (level == "Run") return JEventLevel::Run;
    if (level == "Subrun") return JEventLevel::Subrun;
    if (level == "Timeslice") return JEventLevel::Timeslice;
    if (level == "Block") return JEventLevel::Block;
    if (level == "SlowControls") return JEventLevel::SlowControls;
    if (level == "PhysicsEvent") return JEventLevel::PhysicsEvent;
    if (level == "Subevent") return JEventLevel::Subevent;
    if (level == "Task") return JEventLevel::Task;
    if (level == "None") return JEventLevel::None;
    throw JException("Invalid JEventLevel '%s'", level.c_str());
}


inline JEventLevel next_level(JEventLevel current_level) {
    switch (current_level) {
        case JEventLevel::Run: return JEventLevel::Subrun;
        case JEventLevel::Subrun: return JEventLevel::Timeslice;
        case JEventLevel::Timeslice: return JEventLevel::Block;
        case JEventLevel::Block: return JEventLevel::SlowControls;
        case JEventLevel::SlowControls: return JEventLevel::PhysicsEvent;
        case JEventLevel::PhysicsEvent: return JEventLevel::Subevent;
        case JEventLevel::Subevent: return JEventLevel::Task;
        case JEventLevel::Task: return JEventLevel::None;
        default: return JEventLevel::None;
    }
}
