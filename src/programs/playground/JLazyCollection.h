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

#ifndef JANA2_JFACTORYSET2_H
#define JANA2_JFACTORYSET2_H


#include "JAbstractFactory.h"
#include "JFactoryGenerator2.h"

#include <iostream>
#include <map>
#include <typeindex>

template <typename InType>
class JLazyCollection {

private:
    InType* m_typed_this;

    std::map<std::pair<std::type_index, std::string>,
             std::pair<FactoryGenerator<InType>*, JAbstractFactory<InType>*>> m_underlying;

public:

    JLazyCollection(InType* typed_this) : m_typed_this(typed_this) {}

    JLazyCollection(const std::vector<FactoryGenerator<InType>*>& factory_generators) {
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

    ~JLazyCollection() {
        Clear();
    }

    void Update() {
        // Whenever a run number or calibration? changes
        for (auto& row : m_underlying) {
            row.second.second->invalidate_metadata();
        }
    };

    void Clear() {
        // Whenever context gets recycled.
        for (auto& row : m_underlying) {
            row.second.second->clear();
        }
    };


    // Factory get/create
    template<typename T>
    JAbstractFactoryT<T, InType>* GetFactory(std::string tag="", bool create_dummy = false) {

        auto key = std::make_pair(std::type_index(typeid(T)), tag);
        auto it = m_underlying.find(key);
        if (it != m_underlying.end()) {

            if (it->second.second != nullptr) {
                // We have the factory, so we can just return it
                return static_cast<JAbstractFactoryT<T, InType>*>(it->second.second);
            }
            else {
                // We don't have the factory, but we can create it
                auto fac = it->second.first->create(tag);
                it->second.second = fac;
                return static_cast<JAbstractFactoryT<T, InType>*>(fac);
            }
        }
        else if (create_dummy) {
            // We don't have a generator, but we are allowed to create a dummy
            auto fac = new JAbstractFactoryT<T, InType>(tag);
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
    JAbstractFactoryT<T, InType>* Get(T** destination, const std::string& tag="") {

        auto factory = GetFactory<T>(tag);
        auto iterators = factory->get_or_create(*m_typed_this);
        if (std::distance(*iterators.first, *iterators.second) != 1) {
            throw JException("Wrong number of elements!");
        }
        *destination = *iterators.first;
        return factory;
    }

    template<class T>
    JAbstractFactoryT<T, InType>* Get(std::vector<T*>& destination, const std::string& tag = "") {

        auto factory = GetFactory<T>(tag);
        auto iterators = factory->get_or_create(*m_typed_this);
        for (auto it=iterators.first; it!=iterators.second; it++) {
            destination.push_back(*it);
        }
        return factory;
    }


    // C++ style getters
    template<class T>
    const T* GetSingle(const std::string& tag = "") {

        auto iterators = GetFactory<T>(tag)->get_or_create(*m_typed_this);
        if (std::distance(*iterators.first, *iterators.second) != 1) {
            throw JException("Wrong number of elements!");
        }
        return *iterators.first;
    }

    template<class T>
    std::vector<const T*> GetVector(const std::string& tag = "") {

        auto iters = GetFactory<T>(tag)->get_or_create(*m_typed_this);
        std::vector<const T*> vec;
        for (auto it=iters.first; it!=iters.second; ++it) {
            vec.push_back(*it);
        }
        return vec; // Assumes RVO
    }

    template<class T>
    typename JAbstractFactoryT<T,InType>::PairType GetIterators(const std::string& tag = "") {

        return GetFactory<T>(tag)->get_or_create(*m_typed_this);
    }
};

template <typename InType>
class JLazyCollectionMutable : virtual public JLazyCollection<InType> {
public:

    JLazyCollectionMutable(InType* typed_this) : JLazyCollection<InType>(typed_this){

    }

    // Insert
    template <typename T>
    void Insert(T* item, const std::string& tag = "") {

        // This is conceptually `this->GetFactory<T>(tag, true)`
        auto fac = JLazyCollection<InType>::template GetFactory<T>(tag, true);
        fac->insert(item);
    }

    template <typename T>
    void Insert(const std::vector<T*>& items, const std::string& tag = "") {

        // This is conceptually `this->GetFactory<T>(tag, true)`
        auto fac = JLazyCollection<InType>::template GetFactory<T>(tag, true);
        for (T* item : items) {
            fac->insert(item);
        }
    }
    void Insert() { std::cout << "Calling Insert" << std::endl; }
};



#endif //JANA2_JFACTORYSET2_H



