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

#ifndef JANA2_JARROWTOPOLOGY_H
#define JANA2_JARROWTOPOLOGY_H


#include <JANA/Services/JComponentManager.h>
#include <JANA/Status/JPerfMetrics.h>
#include <JANA/Utils/JProcessorMapping.h>
#include <JANA/Utils/JEventPool.h>

#include "JActivable.h"
#include "JArrow.h"
#include "JMailbox.h"


struct JArrowTopology : public JActivable {

    using Event = std::shared_ptr<JEvent>;
    using EventQueue = JMailbox<Event>;

    enum Status { Inactive, Running, Draining, Finished };

    explicit JArrowTopology();
    virtual ~JArrowTopology();

    static JArrowTopology* from_components(std::shared_ptr<JComponentManager>, std::shared_ptr<JParameterManager>, int nthreads);

    std::shared_ptr<JComponentManager> component_manager;
    // Ensure that ComponentManager stays alive at least as long as JArrowTopology does
    // Otherwise there is a potential use-after-free when JArrowTopology or JArrowProcessingController access components

    std::shared_ptr<JEventPool> event_pool; // TODO: Belongs somewhere else
    JPerfMetrics metrics;
    Status status = Inactive; // TODO: Merge this concept with JActivable

    std::vector<JArrow*> arrows;
    std::vector<JArrow*> sources;           // Sources needed for activation
    std::vector<JArrow*> sinks;             // Sinks needed for finished message count // TODO: Not anymore
    std::vector<EventQueue*> queues;        // Queues shared between arrows
    JProcessorMapping mapping;

    JLogger _logger;

    bool is_active() override;
    void set_active(bool is_active) override;
};


#endif //JANA2_JARROWTOPOLOGY_H
