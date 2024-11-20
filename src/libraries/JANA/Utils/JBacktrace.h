
// Copyright 2024, Jefferson Science Associates, LLC.
// Subject to the terms in the LICENSE file found in the top-level directory.

#pragma once

#include <ostream>
#include <string>
#include <atomic>


class JBacktrace {
    static const int MAX_FRAMES = 100;
    void* m_buffer[MAX_FRAMES];
    int m_frame_count = 0;
    int m_frames_to_omit = 0;
    std::atomic_bool m_ready {false};

public:
    void WaitForCapture() const;
    void Capture(int frames_to_omit=0);
    void Reset();
    std::string ToString();
    void Format(std::ostream& os);
    std::string AddrToLineInfo(const char* filename, size_t offset);

};


