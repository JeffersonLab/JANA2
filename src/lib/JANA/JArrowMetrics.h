//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
//
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
// This material resulted from work developed under a United States Government Contract.
// The Government retains a paid-up, nonexclusive, irrevocable worldwide license in such
// copyrighted data to reproduce, distribute copies to the public, prepare derivative works,
// perform publicly and display publicly and to permit others to do so.
// THIS SOFTWARE IS PROVIDED BY JEFFERSON SCIENCE ASSOCIATES LLC "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
// JEFFERSON SCIENCE ASSOCIATES, LLC OR THE U.S. GOVERNMENT BE LIABLE TO LICENSEE OR ANY
// THIRD PARTES FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
// OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
//
// Author: Nathan Brei
//

#ifndef JANA2_JARROWMETRIC_H
#define JANA2_JARROWMETRIC_H

#include <mutex>
#include <chrono>

class JArrowMetrics {

public:
    enum class Status {KeepGoing, ComeBackLater, Finished, NotRunYet, Error};
    using duration_t = std::chrono::steady_clock::duration;

private:
    mutable std::mutex _mutex;
    // Mutex is mutable so that we can lock before reading from a const ref

    Status _last_status;
    size_t _total_message_count;
    size_t _last_message_count;
    size_t _total_queue_visits;
    size_t _last_queue_visits;
    duration_t _total_latency;
    duration_t _last_latency;
    duration_t _total_queue_latency;
    duration_t _last_queue_latency;


    // TODO: We might want to add a timestamp, so that
    // the 'last_*' measurements can reflect the most recent value,
    // rather than the last-to-be-accumulated value.

public:
    void clear() {

        _mutex.lock();
        _last_status = Status::NotRunYet;
        _total_message_count = 0;
        _last_message_count = 0;
        _total_queue_visits = 0;
        _last_queue_visits = 0;
        _total_latency = duration_t::zero();
        _last_latency = duration_t::zero();
        _total_queue_latency = duration_t::zero();
        _last_queue_latency = duration_t::zero();
        _mutex.unlock();
    }

    void take(JArrowMetrics& other) {

        _mutex.lock();
        other._mutex.lock();

        if (other._last_message_count != 0) {
            _last_message_count = other._last_message_count;
            _last_latency = other._last_latency;
        }

        _last_status = other._last_status;
        _total_message_count += other._total_message_count;
        _total_queue_visits += other._total_queue_visits;
        _last_queue_visits = other._last_queue_visits;
        _total_latency += other._total_latency;
        _total_queue_latency += other._total_queue_latency;
        _last_queue_latency = other._last_queue_latency;

        other._last_status = Status::NotRunYet;
        other._total_message_count = 0;
        other._last_message_count = 0;
        other._total_queue_visits = 0;
        other._last_queue_visits = 0;
        other._total_latency = duration_t::zero();
        other._last_latency = duration_t::zero();
        other._total_queue_latency = duration_t::zero();
        other._last_queue_latency = duration_t::zero();
        other._mutex.unlock();
        _mutex.unlock();
    };

    void update(const JArrowMetrics &other) {

        _mutex.lock();
        other._mutex.lock();

        if (other._last_message_count != 0) {
            _last_message_count = other._last_message_count;
            _last_latency = other._last_latency;
        }
        _total_latency += other._total_latency;
        _last_status = other._last_status;
        _total_message_count += other._total_message_count;
        _total_queue_visits += other._total_queue_visits;
        _last_queue_visits = other._last_queue_visits;
        _total_queue_latency += other._total_queue_latency;
        _last_queue_latency = other._last_queue_latency;
        other._mutex.unlock();
        _mutex.unlock();
    };

    void update_finished() {
        _mutex.lock();
        _last_status = Status::Finished;
        _mutex.unlock();
    }

    void update(const Status& last_status,
                const size_t& message_count_delta,
                const size_t& queue_visit_delta,
                const duration_t& latency_delta,
                const duration_t& queue_latency_delta) {

        _mutex.lock();
        _last_status = last_status;

        if (message_count_delta > 0) {
            // We don't want to lose our most recent latency numbers
            // when the most recent execute() encounters an empty
            // queue and consequently processes zero items.
            _last_message_count = message_count_delta;
            _last_latency = latency_delta;
        }
        _total_message_count += message_count_delta;
        _total_queue_visits += queue_visit_delta;
        _last_queue_visits = queue_visit_delta;
        _total_latency += latency_delta;
        _total_queue_latency += queue_latency_delta;
        _last_queue_latency = queue_latency_delta;
        _mutex.unlock();

    };

    void get(Status& last_status,
             size_t& total_message_count,
             size_t& last_message_count,
             size_t& total_queue_visits,
             size_t& last_queue_visits,
             duration_t& total_latency,
             duration_t& last_latency,
             duration_t& total_queue_latency,
             duration_t& last_queue_latency) {

        _mutex.lock();
        last_status = _last_status;
        total_message_count = _total_message_count;
        last_message_count = _last_message_count;
        total_queue_visits = _total_queue_visits;
        last_queue_visits = _last_queue_visits;
        total_latency = _total_latency;
        last_latency = _last_latency;
        total_queue_latency = _total_queue_latency;
        last_queue_latency = _last_queue_latency;
        _mutex.unlock();
    }

    size_t get_total_message_count() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _total_message_count;
    }

    Status get_last_status() {
        std::lock_guard<std::mutex> lock(_mutex);
        return _last_status;
    }
};

inline std::string to_string(JArrowMetrics::Status h) {
    switch (h) {
        case JArrowMetrics::Status::KeepGoing:     return "KeepGoing";
        case JArrowMetrics::Status::ComeBackLater: return "ComeBackLater";
        case JArrowMetrics::Status::Finished:      return "Finished";
        case JArrowMetrics::Status::NotRunYet:     return "NotRunYet";
        case JArrowMetrics::Status::Error:
        default:                          return "Error";
    }
}

#endif //JANA2_JARROWMETRIC_H
