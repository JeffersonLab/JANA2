
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <ostream>
#include <string>
#include <atomic>
#include <array>


class JBacktrace {
    static const int MAX_FRAMES = 100;
    std::array<void*, MAX_FRAMES> m_buffer;
    int m_frame_count = 0;
    int m_frames_to_omit = 0;
    std::atomic_bool m_ready {false};

public:

    JBacktrace() = default;
    ~JBacktrace() = default;
    JBacktrace(const JBacktrace& other);
    JBacktrace(JBacktrace&& other);
    JBacktrace& operator=(const JBacktrace&);

    void WaitForCapture() const;
    void Capture(int frames_to_omit=0);
    void Reset();
    std::string ToString() const;
    void Format(std::ostream& os) const;
    std::string AddrToLineInfo(const char* filename, size_t offset) const;

};


