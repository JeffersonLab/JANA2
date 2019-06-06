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
#include <JANA/JException.h>

class JContext;


/// Basic template for factory metadata. T <: JObject.
/// This definition gets overridden using template specialization.
/// Note that each JObject type has exactly _one_ metadata definition.
/// Anything else would break plugin substitutability.
template <typename T>
struct Metadata {};


/// Proposal for next generation of Factories.
/// The base class doesn't do anything (except declare its own type information,
/// track its lifecycle, and offer virtual functions for the user to override.
class Factory {

    const std::string m_tag;
    const std::string m_type_name;
    const std::string m_inner_type_name;
    const std::type_index m_inner_type_index;

protected:

    enum class Status {Uninitialized, InvalidMetadata, Unprocessed, Processed};
    Status m_status = Status::Uninitialized;
    std::mutex m_mutex;

public:
    Factory(std::string inner_type_name, std::type_index inner_type_index, std::string tag = "");

    std::string get_tag() { return m_tag; }
    std::string get_type_name() { return m_type_name; }
    std::string get_inner_type_name() { return m_inner_type_name; }
    std::type_index get_inner_type_index() { return m_inner_type_index; }


    /// These are meant to be overridden by the user.
    /// The user should call insert() and get_metadata()
    virtual void init() {};
    virtual void update(JContext& c) {};   // E.g. on change of run number
    virtual void process(JContext& c) {};  // Does not need to be threadsafe

    /// Allows an upstream caller to indicate that the metadata needs an update,
    /// e.g. if the run number has changed. Once this has been set, the next call
    /// to retrieve() will in turn call update() and then process().
    void invalidate_metadata() {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_status != Status::Uninitialized) {
            m_status = Status::InvalidMetadata;
        }
    }

    /// This is meant to be overridden by FactoryT, but not by the user.
    virtual void clear() {};

};


/// How are non-dummy Factories used?
/// - User supplies a Metadata specialization and a process() implementation.
/// - Inside process(), the user will call get_metadata() and insert()
/// - Externally, JANA will call get(), update(), clear()
/// - Externally, the user will call get_metadata()
/// - Externally, NOBODY will call insert()

/// How are dummy Factories used?
/// - Users may substitute a real Factory with a dummy one any time.
/// - Externally, JANA calls insert(), get().
/// - Externally, the user will call get_metadata() (because of substitutability)
/// - Internally, nobody calls anything.

template <typename T>
class FactoryT : public Factory {

public:
    using iterator_t = typename std::vector<T*>::const_iterator;
    using pair_t = std::pair<iterator_t, iterator_t>;

private:
    std::vector<T*> m_underlying;
    Metadata<T> m_metadata;

public:
    explicit FactoryT(std::string tag = "")
        : Factory("asdf", std::type_index(typeid(T)), std::move(tag)) {
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

    // TODO: Figure out best return type
    Metadata<T>& get_metadata() { return m_metadata; }

    // TODO: This will deadlock if there is a cycle in our Factory dependencies
    // TODO: This will be inefficient if we have multiple threads hammering one Context
    pair_t get_or_create(JContext& c) {

        std::lock_guard<std::mutex> lock(m_mutex);
        switch (m_status) {
            case Status::Uninitialized:
                init();
            case Status::InvalidMetadata:
                update(c);
            case Status::Unprocessed:
                process(c);
                m_status = Status::Processed;
            case Status::Processed:
                return std::make_pair(m_underlying.cbegin(), m_underlying.cend());
            default:
                throw JException("How did we end up here?");
        }
    }

    void clear() override {
        for (auto t : m_underlying) {
            delete t;
        }
        m_underlying.clear();
    };


};



#endif //JANA2_FACTORY_H
