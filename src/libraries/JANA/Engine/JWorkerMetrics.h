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

#ifndef JANA2_JWORKERMETRICS_H
#define JANA2_JWORKERMETRICS_H

#include <chrono>
#include <mutex>

class JWorkerMetrics {
    /// Workers need to maintain metrics. Similar to Arrow::Metrics, these form a monoid
    /// where the identity element is (0,0,0) and the combine operation accumulates totals
    /// in a thread-safe way. Alas, the combine operation mutates state for performance reasons.
    /// We've separated Metrics from the Worker itself because it is not always obvious
    /// who should be performing the accumulation or when, and this gives us the freedom to
    /// try different possibilities.


    using duration_t = std::chrono::steady_clock::duration;

    mutable std::mutex _mutex;
    // mutex is mutable so that we can lock before reading from a const ref

    long _scheduler_visit_count;

    duration_t _total_useful_time;
    duration_t _total_retry_time;
    duration_t _total_scheduler_time;
    duration_t _total_idle_time;
    duration_t _last_useful_time;
    duration_t _last_retry_time;
    duration_t _last_scheduler_time;
    duration_t _last_idle_time;

public:
    void clear() {
        _mutex.lock();
        _scheduler_visit_count = 0;

        auto zero = duration_t::zero();

        _total_useful_time = zero;
        _total_retry_time = zero;
        _total_scheduler_time = zero;
        _total_idle_time = zero;
        _last_useful_time = zero;
        _last_retry_time = zero;
        _last_scheduler_time = zero;
        _last_idle_time = zero;
        _mutex.unlock();
    }


    void update(const JWorkerMetrics &other) {

        _mutex.lock();
        other._mutex.lock();
        _scheduler_visit_count += other._scheduler_visit_count;
        _total_useful_time += other._total_useful_time;
        _total_retry_time += other._total_retry_time;
        _total_scheduler_time += other._total_scheduler_time;
        _total_idle_time += other._total_idle_time;
        _last_useful_time = other._last_useful_time;
        _last_retry_time = other._last_retry_time;
        _last_scheduler_time = other._last_scheduler_time;
        _last_idle_time = other._last_idle_time;
        other._mutex.unlock();
        _mutex.unlock();
    }


    void update(const long& scheduler_visit_count,
                const duration_t& useful_time,
                const duration_t& retry_time,
                const duration_t& scheduler_time,
                const duration_t& idle_time) {

        _mutex.lock();
        _scheduler_visit_count += scheduler_visit_count;
        _total_useful_time += useful_time;
        _total_retry_time += retry_time;
        _total_scheduler_time += scheduler_time;
        _total_idle_time += idle_time;
        _last_useful_time = useful_time;
        _last_retry_time = retry_time;
        _last_scheduler_time = scheduler_time;
        _last_idle_time = idle_time;
        _mutex.unlock();
    }


    void get(long& scheduler_visit_count,
             duration_t& total_useful_time,
             duration_t& total_retry_time,
             duration_t& total_scheduler_time,
             duration_t& total_idle_time,
             duration_t& last_useful_time,
             duration_t& last_retry_time,
             duration_t& last_scheduler_time,
             duration_t& last_idle_time) {

        _mutex.lock();
        scheduler_visit_count = _scheduler_visit_count;
        total_useful_time = _total_useful_time;
        total_retry_time = _total_retry_time;
        total_scheduler_time = _total_scheduler_time;
        total_idle_time = _total_idle_time;
        last_useful_time = _last_useful_time;
        last_retry_time = _last_retry_time;
        last_scheduler_time = _last_scheduler_time;
        last_idle_time = _last_idle_time;
        _mutex.unlock();
    }

};


#endif //JANA2_JWORKERMETRICS_H
