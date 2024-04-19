// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <ostream>
#include <sstream>

enum class JEventLevel { Run, Subrun, Timeslice, Block, PhysicsEvent, Subevent, Task, None };

inline std::ostream& operator<<(std::ostream& os, JEventLevel level) {
    switch (level) {
        case JEventLevel::Run: os << "Run"; break;
        case JEventLevel::Subrun: os << "Subrun"; break;
        case JEventLevel::Timeslice: os << "Timeslice"; break;
        case JEventLevel::Block: os << "Block"; break;
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


inline JEventLevel next_level(JEventLevel current_level) {
    switch (current_level) {
        case JEventLevel::Run: return JEventLevel::Subrun;
        case JEventLevel::Subrun: return JEventLevel::Timeslice;
        case JEventLevel::Timeslice: return JEventLevel::Block;
        case JEventLevel::Block: return JEventLevel::PhysicsEvent;
        case JEventLevel::PhysicsEvent: return JEventLevel::Subevent;
        case JEventLevel::Subevent: return JEventLevel::Task;
        case JEventLevel::Task: return JEventLevel::None;
        default: return JEventLevel::None;
    }
}
