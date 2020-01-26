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

#include "JArrowTopology.h"
#include "JEventProcessorArrow.h"
#include "JEventSourceArrow.h"

bool JArrowTopology::is_active() {
    for (auto arrow : arrows) {
        if (arrow->is_active()) {
            return true;
        }
    }
    return false;
}

void JArrowTopology::set_active(bool active) {
    if (active) {
        if (status == Status::Inactive) {
            status = Status::Running;
            for (auto arrow : sources) {
                // We activate our eventsources, which activate components downstream.
                arrow->initialize();
                arrow->set_active(true);
                arrow->notify_downstream(true);
            }
        }
    }
    else {
        // Someone has told us to deactivate. There are two ways to get here:
        //   * The last JEventProcessorArrow notifies us that he is deactivating because the topology is finished
        //   * The JArrowController is requesting us to stop regardless of whether the topology finished or not

        if (is_active()) {
            // Premature exit: Shut down any arrows which are still running
            status = Status::Draining;
            for (auto arrow : arrows) {
                arrow->set_active(false);
                arrow->finalize();
            }
        }
        else {
            // Arrows have all deactivated: Stop timer
            metrics.stop();
            status = Status::Inactive;
            for (auto arrow : arrows) {
                arrow->finalize();
            }
        }
    }
}

JArrowTopology::JArrowTopology() = default;

JArrowTopology::~JArrowTopology() {
    for (auto arrow : arrows) {
        delete arrow;
    }
}

JArrowTopology* JArrowTopology::from_components(std::shared_ptr<JComponentManager> jcm, std::shared_ptr<JParameterManager> params) {

    JArrowTopology* topology = new JArrowTopology();

    size_t event_pool_size = 100;
    size_t event_queue_threshold = 80;
    size_t event_source_chunksize = 40;
    size_t event_processor_chunksize = 1;
    size_t location_count = 1;
    bool enable_stealing = false;
    bool limit_total_events_in_flight = false;
    int affinity = 0;
    int locality = 0;

    params->SetDefaultParameter("jana:event_pool_size", event_pool_size);
    params->SetDefaultParameter("jana:limit_total_events_in_flight", limit_total_events_in_flight);
    params->SetDefaultParameter("jana:event_queue_threshold", event_queue_threshold);
    params->SetDefaultParameter("jana:event_source_chunksize", event_source_chunksize);
    params->SetDefaultParameter("jana:event_processor_chunksize", event_processor_chunksize);
    params->SetDefaultParameter("jana:enable_stealing", enable_stealing);
    params->SetDefaultParameter("jana:affinity", affinity);
    params->SetDefaultParameter("jana:locality", locality);

    topology->mapping.initialize(static_cast<JProcessorMapping::AffinityStrategy>(affinity),
                                 static_cast<JProcessorMapping::LocalityStrategy>(locality));

    topology->event_pool = std::make_shared<JEventPool>(&jcm->get_fac_gens(), event_pool_size,
            location_count, limit_total_events_in_flight);

    // Assume the simplest possible topology for now, complicate later
    auto queue = new EventQueue(event_queue_threshold, topology->mapping.get_loc_count(), enable_stealing);

    for (auto src : jcm->get_evt_srces()) {

        JArrow* arrow = new JEventSourceArrow(src->GetName(), src, queue, topology->event_pool);
        arrow->set_backoff_tries(0);
        topology->arrows.push_back(arrow);
        topology->sources.push_back(arrow);
        arrow->set_chunksize(event_source_chunksize);
        // create arrow for sources. Don't open until arrow.activate() called
    }

    auto proc_arrow = new JEventProcessorArrow("processors", queue, nullptr, topology->event_pool);
    proc_arrow->set_chunksize(event_processor_chunksize);
    topology->arrows.push_back(proc_arrow);

    // Receive notifications when sinks finish
    proc_arrow->attach_downstream(topology);
    topology->attach_upstream(proc_arrow);

    for (auto proc : jcm->get_evt_procs()) {
        proc_arrow->add_processor(proc);
    }
    topology->sinks.push_back(proc_arrow);


    return topology;
}
