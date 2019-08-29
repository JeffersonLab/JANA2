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

#ifndef JANA2_JFACTORY2_H
#define JANA2_JFACTORY2_H

/// This has nothing to do with the Abstract Factory design pattern. The name is terrible. Please change.

#include <typeindex>
#include <mutex>
#include <vector>
#include <JANA/Utils/JTypeInfo.h>
#include "JException.h"


template <class InputType>
class JAbstractFactory {

    const std::string m_tag;
    const std::string m_type_name;
    const std::string m_inner_type_name;
    const std::type_index m_inner_type_index;

protected:
    enum class Status {InvalidMetadata, Unprocessed, Processed};
    Status m_status = Status::InvalidMetadata;
    std::mutex m_mutex;

public:
    JAbstractFactory(std::string type_name, std::string tag, std::string inner_type_name, std::type_index inner_type_index)
            : m_type_name(std::move(type_name))
            , m_tag(std::move(tag))
            , m_inner_type_name(std::move(inner_type_name))
            , m_inner_type_index(inner_type_index)
    {
    }

    std::string get_type_name() { return m_type_name; }
    std::string get_tag() { return m_tag; }
    std::string get_inner_type_name() { return m_inner_type_name; }
    std::type_index get_inner_type_index() { return m_inner_type_index; }

    /// These are meant to be overridden by the user.
    /// The user should call insert() and get_metadata()
    virtual void update(InputType in) {};   // E.g. on change of run number
    virtual void process(InputType in) {};  // Does not need to be threadsafe

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



/// Basic template for factory metadata. T <: JObject.
/// This definition gets overridden using template specialization.
/// Note that each JObject type has exactly _one_ metadata definition.
/// Anything else would break plugin substitutability.
template <typename T>
struct Metadata {};




template <typename OutputType, typename InputType>
class JAbstractFactoryT : public JAbstractFactory<InputType> {

public:
    using iterator_t = typename std::vector<OutputType*>::const_iterator;
    using pair_t = std::pair<iterator_t, iterator_t>;

protected:
    std::vector<OutputType*> m_underlying;
    Metadata<OutputType> m_metadata;

public:
    explicit JAbstractFactoryT(std::string type_name = JTypeInfo::demangle<OutputType>(), std::string tag = "")
    : JAbstractFactory<InputType>(type_name, tag, type_name, std::type_index(typeid(OutputType)))
    {
    }
    virtual ~JAbstractFactoryT() = default;

    void process(InputType c) override {};

    void insert(void* t) {
        OutputType* casted = dynamic_cast<OutputType*>(t);
        assert(casted != nullptr);
        m_underlying.push_back(casted);
    }

    void insert(OutputType* t) {
        m_underlying.push_back(t);
    }

    // TODO: Figure out best return type
    Metadata<OutputType>& get_metadata() { return m_metadata; }

    // TODO: This will deadlock if there is a cycle in our Factory dependencies
    pair_t get_or_create(InputType c) {

        // TODO: Use double-checking for less overhead
        std::lock_guard<std::mutex> lock(JAbstractFactory<InputType>::m_mutex);

        switch (JAbstractFactory<InputType>::m_status) {

            case JAbstractFactory<InputType>::Status::InvalidMetadata:
                JAbstractFactory<InputType>::update(c);
            case JAbstractFactory<InputType>::Status::Unprocessed:
                JAbstractFactory<InputType>::process(c);
                process(c);
                JAbstractFactory<InputType>::m_status = JAbstractFactory<InputType>::Status::Processed;
            case JAbstractFactory<InputType>::Status::Processed:
                return std::make_pair(m_underlying.cbegin(), m_underlying.cend());
            default:
                throw JException("Enum is set to a garbage value somehow");
        }
    }

    void clear() final {
        for (auto t : m_underlying) {
            delete t;
        }
        m_underlying.clear();
    };

};



#endif //JANA2_JFACTORY2_H
