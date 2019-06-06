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

#include "JContext.h"

#include <vector>

JContext::JContext(const std::vector<FactoryGenerator*>& factory_generators) {

    // Build underlying table from factory generators
    // Note that factories themselves are instantiated lazily

    for (auto gen : factory_generators) {

        auto key = std::make_pair(gen->get_inner_type_index(), gen->get_tag());
        auto val = std::make_pair(gen, nullptr);
        m_underlying[key] = val;

        // Our strategy for dealing with duplicates is to use the last
        // This is something we want to handle upstream, e.g. in JTopologyBuilder
    }
}

JContext::~JContext() {
    Clear();
}

/// Update() tells all Factories to (eagerly) update any persistent metadata.
/// The only example of this is when there is a change of run number. Someone (either the EventSource
/// or the EventSourceArrow) notices that the run number has changed compared to the previous contents of
/// the Event Context and tells the factories that their metadata is now invalid.

/// TODO: We could make this lazy by setting an 'invalid' bit on m_underlying and
///       calling factory.update() from context.get() instead.

/// TODO: Reconsider storing metadata inside Factories in favor of some FRP pattern

void JContext::Update() {
    for (auto& row : m_underlying) {
        row.second.second->invalidate_metadata();
    }
}

/// Clear() tells all Factories to clear all JObjects associated with this Context.
/// This is usually called when recycling an Event from the EventPool.
/// Note that metadata inside Factories persists across Clear() operations.

void JContext::Clear() {
    for (auto& row : m_underlying) {
        row.second.second->clear();
    }
}


