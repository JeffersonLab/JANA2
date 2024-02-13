// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <ostream>

enum class JEventLevel { Run, Subrun, Timeslice, Block, Event, Subevent, Task };

inline std::ostream& operator<<(std::ostream& os, JEventLevel level) {
    switch (level) {
        case JEventLevel::Run: os << "Run"; break;
        case JEventLevel::Subrun: os << "Subrun"; break;
        case JEventLevel::Timeslice: os << "Timeslice"; break;
        case JEventLevel::Block: os << "Block"; break;
        case JEventLevel::Event: os << "Event"; break;
        case JEventLevel::Subevent: os << "Subevent"; break;
        case JEventLevel::Task: os << "Task"; break;
    }
    return os;
}


