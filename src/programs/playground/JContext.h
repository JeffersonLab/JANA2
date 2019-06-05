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

#ifndef JANA2_JCONTEXT_H
#define JANA2_JCONTEXT_H

#include "FactoryGenerator.h"

#include <memory>
#include <map>

class JContext {
private:
    std::map<std::pair<std::type_index, std::string>, Factory*> m_factories;

public:

    JContext(const std::vector<FactoryGenerator*>& factory_generators);
    ~JContext();
    void Update();  // Whenever a run number or calibration? changes
    void Clear();   // Whenever context gets recycled


    // C style getters
    template<class T>
    FactoryT<T>* Get(T** item, const std::string& tag="") const;

    template<class T>
    FactoryT<T>* Get(std::vector<const T*> &vec, const std::string& tag = "") const;


    // C++ style getters
    template<class T>
    const T* GetSingle(const std::string& tag = "") const;

    template<class T>
    std::vector<const T*> GetVector(const std::string& tag = "") const;

    template<class T>
    typename FactoryT<T>::PairType GetIterators(const std::string& tag = "") const;

    template<typename T>
    FactoryT<T>* GetFactory(std::string tag="");


    // Insert
    template <typename T>
    void Insert(T* item, const std::string& aTag = "") const;

    template <typename T>
    void Insert(const std::vector<T*>& items, const std::string& tag = "") const;

};

// Template definitions
#include "JContext.tcc"


#endif //JANA2_JCONTEXT_H
