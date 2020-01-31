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

#ifndef JANA2_JDEBUGPROCESSINGCONTROLLER_H
#define JANA2_JDEBUGPROCESSINGCONTROLLER_H

#include <JANA/Services/JProcessingController.h>
#include <JANA/Services/JComponentManager.h>
#include <JANA/Status/JPerfMetrics.h>
#include <JANA/JLogger.h>

#include <thread>

class JDebugProcessingController : public JProcessingController {
public:

    explicit JDebugProcessingController(JComponentManager* jcm) : m_component_manager(jcm) {};
    ~JDebugProcessingController() override;

    void initialize() override;
    void run(size_t nthreads) override;
    void scale(size_t nthreads) override;
    void request_stop() override;
    void wait_until_stopped() override;

    bool is_stopped() override;
    bool is_finished() override;

    std::unique_ptr<const JPerfSummary> measure_performance() override;

    void print_report() override;
    void print_final_report() override;

private:

    void run_worker();

    bool m_stop_requested = false;
    bool m_stop_achieved = false;
    bool m_finish_achieved = false;
    size_t m_total_events_emitted = 0;
    size_t m_total_events_processed = 0;

    std::vector<std::thread*> m_workers;
    JLogger m_logger = JLogger();
    JPerfMetrics m_perf_metrics;
    JComponentManager* m_component_manager = nullptr;

};

#endif //JANA2_JDEBUGPROCESSINGCONTROLLER_H
