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

#ifndef JANA2_FACTORYGENERATOR_H
#define JANA2_FACTORYGENERATOR_H

#include "Factory.h"

#include <typeindex>

/// Proposed new base class for FactoryGenerators.
/// Goals:
/// - Be able to create Factories lazily given (inner_type_index, tag)
/// - Be able to use different Factory impls for different tags
/// - Each Generator instantiates exactly one Factory
/// - Factory creation is decoupled from JFactorySets, etc

struct FactoryGenerator {
    virtual Factory* create(std::string tag) = 0;
    virtual std::type_index get_inner_type_index() = 0;
    virtual std::string get_tag() = 0;
};



/// Proposed default implementation for FactoryGenerators.
/// Goals:
/// - Be able to hide all FactoryGenerator machinery behind a method like
///   builder.add<FactoryType,ObjectType>("tagname")

/// TODO: Statically enforce that F <: FactoryT<T>
template <typename T, typename F = FactoryT<T>>
class FactoryGeneratorT : public FactoryGenerator {

    std::string m_tag;

public:
    explicit FactoryGeneratorT(std::string tag = "") : m_tag(std::move(tag)) {}

    Factory* create(std::string tag) override {
        return new F(tag);
    }
    std::type_index get_inner_type_index() override {
        return std::type_index(typeid(T));
    }
    std::string get_tag() override {
        return m_tag;
    }
};


#endif //JANA2_FACTORYGENERATOR_H
