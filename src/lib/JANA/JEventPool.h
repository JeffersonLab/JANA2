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

#ifndef JANA2_JEVENTPOOL_H
#define JANA2_JEVENTPOOL_H

#include "JEvent.h"
#include "JFactoryGenerator.h"

class JEventPool {
private:

    struct alignas(64) LocalPool {
        std::mutex mutex;
        std::vector<std::shared_ptr<JEvent>> events;
    };

    JApplication* m_app;
    std::vector<JFactoryGenerator*>* m_generators;
    size_t m_pool_size;
    size_t m_location_count;
    std::unique_ptr<LocalPool[]> m_pools;

public:
    inline JEventPool(JApplication* app, std::vector<JFactoryGenerator*>* generators,
                      size_t pool_size, size_t location_count)
        : m_app(app)
        , m_generators(generators)
        , m_pool_size(pool_size)
        , m_location_count(location_count) {

        assert(m_location_count >= 1);
        m_pools = std::unique_ptr<LocalPool[]>(new LocalPool[location_count]());
    }

    inline std::shared_ptr<JEvent> get(size_t location) {

        LocalPool& pool = m_pools[location % m_location_count];
        std::lock_guard<std::mutex> lock(pool.mutex);

        if (pool.events.empty()) {
            auto event = std::make_shared<JEvent>(m_app);
            auto factory_set = new JFactorySet(*m_generators);
            event->SetFactorySet(factory_set);
            return event;
        }
        else {
            auto event = std::move(pool.events.back());
            pool.events.pop_back();
            return event;
        }
    }

    inline void put(std::shared_ptr<JEvent>& event, size_t location) {

        event->mFactorySet->Release();
        LocalPool& pool = m_pools[location % m_location_count];
        std::lock_guard<std::mutex> lock(pool.mutex);

        if (pool.events.size() < m_pool_size) {
            pool.events.push_back(std::move(event));
        }
    }
};


#endif //JANA2_JEVENTPOOL_H
