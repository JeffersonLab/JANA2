// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/JException.h>
#include <ostream>
#include <sstream>
#include <vector>
#include <map>

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

inline char toChar(JEventLevel level) {
    switch (level) {
        case JEventLevel::Run: return 'R';
        case JEventLevel::Subrun: return 'r';
        case JEventLevel::Timeslice: return 'T';
        case JEventLevel::Block: return 'B';
        case JEventLevel::SlowControls: return 'C';
        case JEventLevel::PhysicsEvent: return 'P';
        case JEventLevel::Subevent: return 'p';
        case JEventLevel::Task: return 't';
        default: return 'X';
    }
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

class JEventLevelHierarchy {
    std::vector<JEventLevel> m_bottom_levels;
    std::vector<JEventLevel> m_parent_levels;
    std::vector<JEventLevel> m_all_levels;
    std::map<JEventLevel, std::vector<JEventLevel>> parents;
    std::map<JEventLevel, std::vector<JEventLevel>> children;

private:
    void ToString(std::stringstream& ss, JEventLevel current) {
        ss << current;
        auto it = parents.find(current);
        if (it != parents.end()) {
            ss << "(";
            size_t parent_count = it->second.size();
            for (size_t i=0; i<parent_count; ++i) {
                ToString(ss, it->second[i]);
                if (i != parent_count-1) {
                    ss << ", ";
                }
            }
            ss << ")";
        }
    }

public:

    const std::vector<JEventLevel>& GetAllLevels() const {
        return m_all_levels;
    }
    const std::vector<JEventLevel>& GetParentLevels() const {
        return m_parent_levels;
    }
    const std::vector<JEventLevel>& GetBottomLevels() const {
        return m_bottom_levels;
    }
    void AddBottomLevel(JEventLevel level) {
        m_all_levels.push_back(level);
        m_bottom_levels.push_back(level);
    }
    void AddParentLevel(JEventLevel child, JEventLevel parent) {
        m_all_levels.push_back(parent);
        m_parent_levels.push_back(parent);
        parents[child].push_back(parent);
        children[parent].push_back(child);
    }
    std::string ToString() {
        std::stringstream ss;
        for (size_t i=0; i<m_bottom_levels.size(); ++i) {
            ToString(ss, m_bottom_levels[i]);
            if (i != m_bottom_levels.size()-1) {
                ss << ", ";
            }
        }
        return ss.str();
    }
};
