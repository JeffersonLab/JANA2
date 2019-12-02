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

#include "catch.hpp"

#include "JEventSourceTests.h"

#include <JANA/Components/JEventSourceFrontend.h>
#include <JANA/Components/V2/JEventSourceV2Backend.h>
#include <JANA/JEvent.h>

using jana::v2::JEventSourceV2Backend;
using jana::components::JEventSourceBackend;

TEST_CASE("JEventSourceTests") {

    auto event = std::make_shared<JEvent>();

    DummyFrontend frontend;
    JEventSourceV2Backend backend(&frontend);

    SECTION("When backend.open() is called once, frontend.open() is called once") {
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Unopened);
        backend.open();
        backend.next(*event);
        REQUIRE(frontend.open_count == 1);
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Opened);
    }

    SECTION("When backend.open() is called multiple times, frontend.open() is called once") {
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Unopened);
        backend.open();
        backend.open();
        backend.open();
        backend.next(*event);
        REQUIRE(frontend.open_count == 1);
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Opened);
    }

    SECTION("When backend.open() isn't called at all, frontend.open() is called from backend.next()") {
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Unopened);
        backend.next(*event);
        REQUIRE(frontend.open_count == 1);
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Opened);
    }

    SECTION("When frontend.next() doesn't throw, backend.next() returns SUCCESS") {
        // frontend is constrained to emit exactly three events
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Unopened);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::Success);
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Opened);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::Success);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::Success);
        REQUIRE(frontend.open_count == 1);
        REQUIRE(frontend.event_count == 3);
        REQUIRE(backend.get_event_count() == 3);
    }

    SECTION("When frontend.next() throws kNoMoreEvents, backend.next() returns FAILURE_FINISHED") {
        // frontend is constrained to emit exactly three events
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Unopened);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::Success);
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Opened);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::Success);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::Success);
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Opened);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::FailureFinished);
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Finished);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::FailureFinished);
        REQUIRE(backend.next(*event) == JEventSourceBackend::Result::FailureFinished);
        REQUIRE(backend.get_status() == JEventSourceBackend::Status::Finished);
        REQUIRE(frontend.open_count == 1);
        REQUIRE(frontend.event_count == 3);
        REQUIRE(backend.get_event_count() == 3);
    }

}

