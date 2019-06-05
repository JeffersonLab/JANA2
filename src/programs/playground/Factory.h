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

#ifndef JANA2_FACTORY_H
#define JANA2_FACTORY_H

#include <string>
#include <vector>
#include <typeindex>
#include <mutex>

class JContext;

class Factory {

    const std::string m_tag;
    const std::string m_type_name;
    const std::string m_inner_type_name;
    const std::type_index m_inner_type_index;

    bool m_is_created = false;
    std::mutex m_mutex;

public:
    Factory(std::string inner_type_name, std::type_index inner_type_index, std::string tag = "");

    std::string get_tag() { return m_tag; }
    std::string get_type_name() { return m_type_name; }
    std::string get_inner_type_name() { return m_inner_type_name; }
    std::type_index get_inner_type_index() { return m_inner_type_index; }


    virtual void init() {};
    virtual void update(JContext& c) {};   // E.g. on change of run number
    virtual void process(JContext& c) {};  // Not threadsafe, provided by user
    virtual void clear() {};

    // Threadsafe wrappers for process()
    bool try_process(JContext& c);
    inline void wait_until_processed(JContext& c);

};


template <typename T>
class FactoryT : public Factory {

public:
    using iterator_t = typename std::vector<T*>::const_iterator;
    using pair_t = std::pair<iterator_t, iterator_t>;

private:
    std::vector<T*> m_underlying;

public:
    explicit FactoryT(std::string tag = "") : Factory("asdf", std::type_index(typeid(T)), std::move(tag)) {
    }

    void process(JContext& c) override {};

    void insert(void* t) {
        T* casted = dynamic_cast<T*>(t);
        assert(casted != nullptr);
        m_underlying.push_back(casted);
    }

    void insert(T* t) {
        m_underlying.push_back(t);
    }

    pair_t get() { return std::make_pair(m_underlying.cbegin(), m_underlying.cend()); }

    void clear() override {
        for (auto t : m_underlying) {
            delete t;
        }
        m_underlying.clear();
    };
};



#endif //JANA2_FACTORY_H
