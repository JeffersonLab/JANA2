// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once
#include <JANA/Utils/JEventLevel.h>
#include <vector>

class JEventKey {
    struct Entry { JEventLevel level, 
                   uint64_t absolute_number, // Starts at zero. For each event emitted at this level, increases by 1
                   uint64_t relative_number, // Starts at zero. For each event emitted at this level BELONGING TO THIS PARENT, increases by 1
                   uint64_t user_number }    // User can set this using whatever convention they like. However it should probably increase monotonically.
                                             // Defaults to absolute_number.

    std::vector<Entry> m_entries; // store in REVERSE order: [self, parent, grandparent, ...]
    int64_t m_run_number = -1;  
    // Set to -1 by default (we need to know when we don't have a run number so that the calibrations db can respond accordingly)
    // User can set this to whatever they want
    // Run number will automatically be propagated to any children

public:

    JEventKey() {
        m_entries.push_back({JEventLevel::None, 0, 0, 0});
    }

    Configure(JEventLevel level, uint64_t absolute_number) {
        m_entries.clear();
        m_entries.push_back({level, absolute_number, absolute_number, absolute_number});
    }

    AddParent(const JEventKey& parent_key, uint64_t relative_number) {
        if (m_entries.size() > 1) {
            throw JException("Event already has a parent");
        }
        for (const auto& entry : parent_key.m_entries) {
            m_entries.push_back(entry);
            // Store a copy of all parent entries locally so that we can more reliably detect refcount/recycling issues
            // This may change as we figure out how to track non-parent ancestors and siblings
        }
        m_entries[0].relative_number = relative_number;
        if (m_run_number == -1) {
            m_run_number = parent_key.m_run_number;
        }
    }

    const std::vector<Entry>& GetEntries() const {
        return m_entries;
    }

    void SetUserEventNumber(uint64_t user_number) {
        // m_entries always has at least one entry
        if (m_entries.size() == 0) throw JException("")
        m_entries.back().user_number = user_number;
    }

    void SetRunNumber(int64_t run_number) {
        m_run_number = run_number;
    }

    std::string toString() {
        std::ostringstream ss;
        ss << *this;
        return ss.str();
    }

    JEventLevel GetLevel() {
        return m_entries[0].level;
    }

    uint64_t GetAbsoluteEventNumber() {
        return m_entries[0].absolute_number;
    }

    uint64_t GetRelativeEventNumber() {
        return m_entries[0].relative_number;
    }

    uint64_t GetUserEventNumber() {
        return m_entries[0].user_number;
    }
}

inline std::ostream& operator<<(std::ostream& in, const JEventKey& key) {
    // Timeslice 34.10.19 (absolute #1234, user #5678)"
    in << key.GetLevel() << " ";
    auto s = m_entries.size();
    for (size_t i=m_entries.size(); i>0; --i) {
        in << m_entries[s-i-1].relative_number << ".";
    }
    in << key.GetRelativeEventNumber();
    if (key.GetRelativeEventNumber() != key.GetAbsoluteEventNumber()) {
        in << "(absolute #" << key.GetAbsoluteEventNumber();

    }
    if (key.GetUserEventNumber() != key.GetAbsoluteEventNumber()) {
        in << ", user #" << key.GetUserNumber();
    }
    in << ")";
    return in;
}
