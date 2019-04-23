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

#include<JEventTests.h>

#include <JANA/JEvent.h>

#define CATCH_CONFIG_MAIN
#include "catch.hpp"


TEST_CASE("JEventInsertTests") {

    JEvent event;
    event.SetFactorySet(new JFactorySet);

    SECTION("Single-item JEvent::Insert() can be retrieved via JEvent::Get()") {
        auto input = new FakeJObject(22);
        event.Insert(input);
        auto output = event.GetSingle<FakeJObject>();
        REQUIRE(output->datum == input->datum);

        FakeJObject* alternative;
        event.Get(&alternative);
        REQUIRE(output->datum == alternative->datum);

    }

    SECTION("Multi-item JEvent::Insert() can be retrieved via JEvent::Get()") {
        std::vector<FakeJObject*> input;          // const is optional for input
        input.push_back(new FakeJObject(1));
        input.push_back(new FakeJObject(2));
        input.push_back(new FakeJObject(3));
        event.Insert(input);

        std::vector<const FakeJObject*> output;   // const is required for output
        event.Get(output);

        REQUIRE(output.size() == input.size());
        for (size_t i=0; i<output.size(); ++i) {
            REQUIRE(output[i]->datum == input[i]->datum);
        }
    }

    SECTION("JEvent::Insert() respects both tag and typeid") {
        event.Insert(new FakeJObject(22), "first_tag");
        event.Insert(new FakeJObject(49), "second_tag");
        event.Insert(new DifferentFakeJObject(7.6), "first_tag");

        std::vector<const FakeJObject*> output;

        event.Get(output, "first_tag");
        REQUIRE(output.size() == 1);
        REQUIRE(output[0]->datum == 22);

        output.clear();
        event.Get(output, "second_tag");
        REQUIRE(output.size() == 1);
        REQUIRE(output[0]->datum == 49);

        std::vector<const DifferentFakeJObject*> different_output;

        event.Get(different_output, "first_tag");
        REQUIRE(different_output.size() == 1);
        REQUIRE(different_output[0]->sample == 7.6);
    }

    SECTION("Repeated calls to JEvent::Insert() aggregate") {

        event.Insert(new FakeJObject(22), "first_tag");
        event.Insert(new FakeJObject(49), "first_tag");
        event.Insert(new FakeJObject(618), "first_tag");

        std::vector<const FakeJObject*> output;
        event.Get(output, "first_tag");
        REQUIRE(output.size() == 3);
        REQUIRE(output[0]->datum == 22);
        REQUIRE(output[1]->datum == 49);
        REQUIRE(output[2]->datum == 618);
    }

    SECTION("Inserted JObjects get deleted when enclosing JEvent does") {
        FakeJObject* obj = new FakeJObject(618);
        bool deleted = false;
        obj->deleted = &deleted;
        JEvent* event_ptr = new JEvent();
        event_ptr->SetFactorySet(new JFactorySet);
        event_ptr->Insert(obj, "tag");
        REQUIRE(deleted == false);
        delete event_ptr;
        REQUIRE(deleted == true);
    }

}

