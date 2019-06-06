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

#ifndef JANA2_JCONTEXT_TCC
#define JANA2_JCONTEXT_TCC

#include <JANA/JException.h>

#include "JContext.h"


/// GetFactory() will create a FactoryT
template<typename T>
FactoryT<T>* JContext::GetFactory(std::string tag, bool create_dummy) {

    auto key = std::make_pair(std::type_index(typeid(T)), tag);
    auto it = m_underlying.find(key);
    if (it != m_underlying.end()) {

        if (it->second.second != nullptr) {
            // We have the factory, so we can just return it
            return static_cast<FactoryT<T>*>(it->second.second);
        }
        else {
            // We don't have the factory, but we can create it
            auto fac = it->second.first->create(tag);
            it->second.second = fac;
            return static_cast<FactoryT<T>*>(fac);
        }
    }
    else if (create_dummy) {
        // We don't have a generator, but we are allowed to create a dummy
        auto fac = new FactoryT<T>(tag);
        m_underlying[key] = std::make_pair(nullptr, fac);
        return fac;
    }
    else {
        // No generator, and no dummies allowed
        throw JException("Could not find factory!");
    }
}


// C style getters
template<class T>
FactoryT<T>* JContext::Get(T** destination, const std::string& tag) const {

    auto factory = GetFactory<T>(tag)->get_or_create(*this);
    auto iterators = factory.get();
    *destination = *iterators.first;
    // TODO: Assert exactly one element
    return factory;
}

template<class T>
FactoryT<T>* JContext::Get(std::vector<const T*> &destination, const std::string& tag) const {

    auto iterators = GetFactory<T>(tag)->get_or_create(*this);
    for (auto it=iterators.first; it!=iterators.second; it++) {
        destination.push_back(*it);
    }
    return GetFactory<T>(tag);
}


// C++ style getters
template<class T>
const T* JContext::GetSingle(const std::string& tag) const {
    auto result = GetFactory<T>(tag)->get_or_create(*this);
    // TODO: Assert exactly one element
    return *result.first;
}

template<class T>
std::vector<const T*> JContext::GetVector(const std::string& tag) {

    auto iters = GetFactory<T>(tag)->get_or_create(*this);
    std::vector<const T*> vec;
    for (auto it=iters.first; it!=iters.second; ++it) {
        vec.push_back(*it);
    }
    return vec; // Assumes RVO
}

template<class T>
typename FactoryT<T>::PairType JContext::GetIterators(const std::string& tag) const {
    return GetFactory<T>(tag)->get_or_create(*this);
}


/// TODO: Consider a tagged union of (*FactoryGenerator, *Factory, *DummyFactory)
///       Advantages: Detect and complain when user attempts to insert data into a non-dummy factory
///                   No reason to store FactoryGenerators once Factory has been generated

// Insert
template <typename T>
void JContext::Insert(T* item, const std::string& tag) {

    auto fac = GetFactory<T>(tag, true);
    fac.insert(item);
}

template <typename T>
void JContext::Insert(const std::vector<T*>& items, const std::string& tag) {

    auto fac = GetFactory<T>(tag, true);
    for (T* item : items) {
        fac->insert(item);
    }
}


#endif //JANA2_JCONTEXT_TCC
