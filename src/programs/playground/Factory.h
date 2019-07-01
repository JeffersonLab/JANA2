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
///
/// The base class doesn't do much, just declare its own type information,
/// track its lifecycle. We keep around virtual functions which don't need type information,
/// like process(), just in case we need them someday, although it is tempting to get rid of them
///
/// The idea of 'update' generalizes the existing ChangeRunNumber(), so
/// that we can have Contexts which are not Events.
///
/// We remove the function init() because it suggests that the user should do things there
/// like open long-lived resources. As evidenced by the lack of a corresponding finalize()
/// function, there is no mechanism to detect that we are done with a Context except its
/// own destruction. To make things more confusing, we _do_ have a clear() function, but
/// this is called every time the Context gets reused, which happens more often than
/// update(), which happens more often than init(), which should happen
/// only once. Anything that should happen only once should live in the ctor/dtor,
/// things that happen on every Event should live in process(), and things that happen
/// occasionally should live in update().
///
class Factory {

    const std::string m_tag;
    const std::string m_type_name;
    const std::string m_inner_type_name;
    const std::type_index m_inner_type_index;

protected:

    enum class Status {InvalidMetadata, Unprocessed, Processed};
    Status m_status = Status::InvalidMetadata;
    std::mutex m_mutex;

public:
    Factory(std::string inner_type_name, std::type_index inner_type_index, std::string tag = "");

    std::string get_tag() { return m_tag; }
    std::string get_type_name() { return m_type_name; }
    std::string get_inner_type_name() { return m_inner_type_name; }
    std::type_index get_inner_type_index() { return m_inner_type_index; }


    /// These are meant to be overridden by the user.
    /// The user should call insert() and get_metadata()
    virtual void update(JContext& c) {};   // E.g. on change of run number
    virtual void process(JContext& c) {};  // Does not need to be threadsafe

    /// Allows an upstream caller to indicate that the metadata needs an update,
    /// e.g. if the run number has changed. Once this has been set, the next call
    /// to retrieve() will in turn call update() and then process().
    void invalidate_metadata() {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_status = Status::InvalidMetadata;
    }

    /// This is meant to be overridden by FactoryT, but not by the user.
    virtual void clear() {};

};

/// FactoryT is the simplest Factory which has type information attached.
/// The user may inherit from this, or they may create default implementations
/// by using template specialization. This avoids boilerplate
/// but may frighten unsophisticated users. JANA also uses FactoryT in order
/// to automatically create "dummy factories" which act as simple storage
/// containers for sequences of T*.

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

/// This diverges from the existing FactoryT abstraction in one significant way:
/// We introduce Metadata<T> to hold any (zero-dimensional) data associated with
/// the computation of the (one-dimensional) sequence of T. This is needed because
/// we need to enforce that there is exactly one Metadata type per T. If we don't
/// do this, we break the principle of substitutability, which is key to the
/// soundness of our plugin architecture.

template <typename T>
class FactoryT : public Factory {

public:
    using iterator_t = typename std::vector<T*>::const_iterator;
    using pair_t = std::pair<iterator_t, iterator_t>;

protected:
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
    pair_t get_or_create(JContext& c) {

        std::lock_guard<std::mutex> lock(m_mutex);
        switch (m_status) {
            case Status::InvalidMetadata:
                update(c);
            case Status::Unprocessed:
                process(c);
                m_status = Status::Processed;
            case Status::Processed:
                return std::make_pair(m_underlying.cbegin(), m_underlying.cend());
            default:
                throw JException("Enum is set to a garbage value somehow");
        }
    }

    void clear() override {
        for (auto t : m_underlying) {
            delete t;
        }
        m_underlying.clear();
    };


};


/// Proposal for a streamlined Factory API. Users would be steered towards using this,
/// but still be allowed to extend JFactoryT as a fallback.
///
/// Advantages:
/// - We are free to change around our implementations without breaking user code
/// - Everything the user should interact with is exposed as a parameter to a function
/// - User is encouraged to think in terms of inputs and outputs instead of mutating state
/// - We are still mutating state for performance

/// Rough edges:
/// - We are exposing a vector reference to the user, as opposed to a back_inserter,
///     which gives the user freedom to do weird things (like build up their JObjects over
///     multiple passes) but doesn't make the actual contract clear. By "actual contract" I mean
///     "User returns a sequence of JObject pointers which _we_ own"

/// - Semantically, BasicFactory should probably be a Concept, not an abstract class.
///     However, abstract classes make sense to everybody and yield good error messages,
///     and concepts don't really even exist, and they yield absolutely dismal error messages.

/// - If the user wants to avoid the extra virtual function calls (aka be a Concept instead of an
///     abstract class), they just have to remove the part where they inherit from BasicFactory.
///     The BasicFactoryBackend template will work just fine.

/// - There is probably a trick involving std::ref which would let the caller decide whether
///     we are mutating existent state vs a copy of existent state, which could let us be
///     purely functional under the hood

template <typename T>
struct BasicFactory {

    virtual void process(JContext& context, Metadata<T>& metadata, std::vector<T*>& output) = 0;

    virtual void update(JContext& context, Metadata<T>& metadata) {};
};


/// The BasicFactoryBackend provides the linkage between the BasicFactory and FactoryT.
/// This allows us to isolate the policy (100% the user's responsibility) from the mechanism
/// for implementing that policy (100% our responsibility), which is less confusing to the user
/// and less restricting to us. We no longer have to write documentation saying "the user is
/// supposed to use these methods, (or worse, protected member variables), and studiously
/// ignore these other methods".
template <typename T, typename P>
struct BasicFactoryBackend : public FactoryT<T>, public P {

    explicit BasicFactoryBackend(std::string tag) : FactoryT<T>(std::move(tag)) {};

    void process(JContext& c) override {
        P::process(c, FactoryT<T>::m_metadata, FactoryT<T>::m_underlying);
    };

    void update(JContext& c) override {
        P::update(c, FactoryT<T>::m_metadata);
    };
};


#endif //JANA2_FACTORY_H
