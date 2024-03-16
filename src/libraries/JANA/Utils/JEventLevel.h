// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <ostream>

enum class JEventLevel { Run, Subrun, Timeslice, Block, Event, Subevent, Task, None };

inline std::ostream& operator<<(std::ostream& os, JEventLevel level) {
    switch (level) {
        case JEventLevel::Run: os << "Run"; break;
        case JEventLevel::Subrun: os << "Subrun"; break;
        case JEventLevel::Timeslice: os << "Timeslice"; break;
        case JEventLevel::Block: os << "Block"; break;
        case JEventLevel::Event: os << "Event"; break;
        case JEventLevel::Subevent: os << "Subevent"; break;
        case JEventLevel::Task: os << "Task"; break;
        case JEventLevel::None: os << "None"; break;
    }
    return os;
}


inline JEventLevel next_level(JEventLevel current_level) {
    switch (current_level) {
        case JEventLevel::Run: return JEventLevel::Subrun;
        case JEventLevel::Subrun: return JEventLevel::Timeslice;
        case JEventLevel::Timeslice: return JEventLevel::Block;
        case JEventLevel::Block: return JEventLevel::Event;
        case JEventLevel::Event: return JEventLevel::Subevent;
        case JEventLevel::Subevent: return JEventLevel::Task;
        case JEventLevel::Task: return JEventLevel::None;
        case JEventLevel::None: return JEventLevel::None;
    }
}
