#!/usr/bin/env python3
from sys import argv

copyright_notice = """//
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// Jefferson Science Associates LLC Copyright Notice:
// Copyright 251 2014 Jefferson Science Associates LLC All Rights Reserved. Redistribution
// and use in source and binary forms, with or without modification, are permitted as a
// licensed user provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this
//    list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this
//    list of conditions and the following disclaimer in the documentation and/or other
//    materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
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
//"""

jobject_template_h = """
{copyright_notice}

#ifndef _{name}_h_
#define _{name}_h_

#include <JANA/JObject.h>

/// JObjects are plain-old data containers for inputs, intermediate results, and outputs.
/// They have member functions for introspection and maintaining associations with other JObjects, but
/// all of the numerical code which goes into their creation should live in a JFactory instead.
/// You are allowed to include STL containers and pointers to non-POD datatypes inside your JObjects,
/// however, it is highly encouraged to keep them flat and include only primitive datatypes if possible.
/// Think of a JObject as being a row in a database table, with event number as an implicit foreign key.

struct {name} : public JObject {{
    int x;     // Pixel coordinates centered around 0,0
    int y;     // Pixel coordinates centered around 0,0
    double E;  // Energy loss in GeV
    double t;  // Time in ms


    /// Make it convenient to construct one of these things
    {name}(int x, int y, double E, double t) : x(x), y(y), E(E), t(t) {{}};


    /// Override className to tell JANA to store the exact name of this class where we can
    /// access it at runtime. JANA provides a NAME_OF_THIS macro so that this will return the correct value
    /// even if you rename the class using automatic refactoring tools.

    const std::string className() const override {{
        return NAME_OF_THIS;
    }}

    /// Override Summarize to tell JANA how to produce a convenient string representation for our JObject.
    /// This can be used called from user code, but also lets JANA automatically inspect its own data. For instance,
    /// adding JCsvWriter<Hit> will automatically generate a CSV file containing each hit. Warning: This is obviously
    /// slow, so use this for debugging and monitoring but not inside the performance critical code paths.

    void Summarize(JObjectSummary& summary) const override {{
        summary.add(x, NAME_OF(x), "%d", "Pixel coordinates centered around 0,0");
        summary.add(y, NAME_OF(y), "%d", "Pixel coordinates centered around 0,0");
        summary.add(E, NAME_OF(E), "%f", "Energy loss in GeV");
        summary.add(t, NAME_OF(t), "%f", "Time in ms");
    }}
}};


#endif // _{name}_h_

"""

jeventsource_template_h = """
{copyright_notice}

#ifndef _{name}_h_
#define  _{name}_h_

#include <JANA/JEventSource.h>

class {name} : public JEventSource {{

public:
    {name}(std::string resource_name, JApplication* app);

    virtual ~{name}() = default;

    void Open() override;

    void GetEvent(std::shared_ptr<JEvent>) override;

}};

#endif // _{name}_h_

"""

jeventsource_template_cc = """

#include "{name}.h"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>

/// Include headers to any JObjects you wish to associate with each event
// #include "hit.h"

/// There are two different ways of instantiating JEventSources
/// 1. Creating them manually and registering them with the JApplication
/// 2. Creating a corresponding JEventSourceGenerator and registering that instead
///    If you have a list of files as command line args, JANA will use the JEventSourceGenerator
///    to find the most appropriate JEventSource corresponding to that filename, instantiate and register it.
///    For this to work, the JEventSource constructor has to have the following constructor arguments:

{name}::{name}(std::string resource_name, JApplication* app) : JEventSource(resource_name, app) {{
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name
}}

void {name}::Open() {{

    /// Open is called exactly once when processing begins.
    
    /// Get any configuration parameters from the JApplication
    // GetApplication()->SetDefaultParameter("{name}:random_seed", m_seed, "Random seed");

    /// For opening a file, get the filename via:
    // std::string resource_name = GetResourceName();
    /// Open the file here!
}}

void {name}::GetEvent(std::shared_ptr <JEvent> event) {{

    /// Calls to GetEvent are synchronized with each other, which means they can
    /// read and write state on the JEventSource without causing race conditions.
    
    /// Configure event and run numbers
    static size_t current_event_number = 1
    event->SetEventNumber(current_event_number++);
    event->SetRunNumber(22);

    /// Insert whatever data was read into the event
    // std::vector<Hit*> hits;
    // hits.push_back(new Hit(0,0,1.0,0));
    // hits.push_back(new Hit(0,1,1.0,0));
    // hits.push_back(new Hit(1,0,1.0,0));
    // hits.push_back(new Hit(1,1,1.0,0));
    // event->Insert(hits);

    /// If you are reading a file of events and have reached the end, terminate the stream like this:
    // // Close file pointer!
    // throw RETURN_STATUS::kNO_MORE_EVENTS;

    /// If you are streaming events and there are no new events in the message queue,
    /// tell JANA that GetEvent() was temporarily unsuccessful like this:
    // throw RETURN_STATUS::kBUSY;
}}

"""

plugin_main = """

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>

#include "{name}Processor.h"

extern "C" {{
void InitPlugin(JApplication* app) {{

    // This code is executed when the plugin is attached.
    // It should always call InitJANAPlugin(app) first, and then do one or more of:
    //   - Read configuration parameters
    //   - Register JFactoryGenerators
    //   - Register JEventProcessors
    //   - Register JEventSourceGenerators (or JEventSources directly)
    //   - Register JServices

    InitJANAPlugin(app);

    LOG << "Loading {name}" << LOG_END;
    app->Add(new {name}Processor);
    // Add any additional components as needed
}}
}}

"""

plugin_cmakelists_txt = """

set ({name}_PLUGIN_SOURCES
		{name}.cc
		{name}Processor.cc
		{name}Processor.h
	)

add_library({name}_plugin SHARED ${{{name}_PLUGIN_SOURCES}})

find_package(JANA REQUIRED)
target_include_directories({name}_plugin PUBLIC ${{JANA_INCLUDE_DIR}})
target_link_libraries({name}_plugin ${{JANA_LIBRARY}})
set_target_properties({name}_plugin PROPERTIES PREFIX "" OUTPUT_NAME "{name}" SUFFIX ".so")
install(TARGETS {name}_plugin DESTINATION plugins)

"""


plugin_findjana_cmake = """
# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindJANA
--------

Finds the JANA library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``JANA::jana``
  The JANA library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``JANA_FOUND``
  True if the system has the JANA library.
``JANA_VERSION``
  The version of the JANA library which was found.
``JANA_INCLUDE_DIRS``
  Include directories needed to use JANA.
``JANA_LIBRARIES``
  Libraries needed to link to JANA.

#]=======================================================================]

if (DEFINED JANA_HOME)
    set(JANA_ROOT_DIR ${JANA_HOME})
    message(STATUS "Using JANA_HOME = ${JANA_ROOT_DIR} (From CMake JANA_HOME variable)")

elseif (DEFINED ENV{JANA_HOME})
    set(JANA_ROOT_DIR $ENV{JANA_HOME})
    message(STATUS "Using JANA_HOME = ${JANA_ROOT_DIR} (From JANA_HOME environment variable)")

else()
    message(FATAL_ERROR "Missing $JANA_HOME")
endif()

set(JANA_VERSION 2)

find_path(JANA_INCLUDE_DIR
        NAMES "JANA/JApplication.h"
        PATHS ${JANA_ROOT_DIR}/include
        )

find_library(JANA_LIBRARY
        NAMES "JANA"
        PATHS ${JANA_ROOT_DIR}/lib
        )

set(JANA_LIBRARIES ${JANA_LIBRARY})
set(JANA_INCLUDE_DIRS ${JANA_ROOT_DIR}/include)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JANA
        FOUND_VAR JANA_FOUND
        VERSION_VAR JANA_VERSION
        REQUIRED_VARS JANA_ROOT_DIR JANA_INCLUDE_DIR JANA_LIBRARY
        )
"""

plugin_tests_cmakelists_txt = """

set ({name}_PLUGIN_TESTS_SOURCES
        catch.hpp
        tests_main.cc
        integration_tests.cc
        # Add component tests here
        )

add_executable({name}_plugin_tests ${{{name}_PLUGIN_TESTS_SOURCES}})

find_package(JANA REQUIRED)

target_include_directories({name}_plugin_tests PUBLIC ../src)
target_include_directories({name}_plugin_tests PUBLIC ${{JANA_INCLUDE_DIR}})

target_link_libraries({name}_plugin_tests {name}_plugin)
target_link_libraries({name}_plugin_tests ${{JANA_LIBRARY}})

install(TARGETS {name}_plugin_tests DESTINATION bin)

"""

plugin_integration_tests_cc = """
#include "catch.hpp"
#include "{name}Processor.h"

#include <JANA/JApplication.h>
#include <JANA/JFactoryGenerator.h>


// This is where you can assemble various components and verify that when put together, they
// do what you'd expect. This means you can skip the laborious mixing and matching of plugins and configurations,
// and have everything run automatically inside one executable.

TEST_CASE("IntegrationTests") {{

    auto app = new JApplication;
    
    // Create and register components
    // app->Add(new {name}Processor);

    // Set test parameters
    app->SetParameterValue("nevents", 10);

    // Run everything, blocking until finished
    app->Run();

    // Verify the results you'd expect
    REQUIRE(app->GetNeventsProcessed() == 10);

}}

"""

plugin_tests_main_cc = """

// This is the entry point for our test suite executable.
// Catch2 will take over from here.

#define CATCH_CONFIG_MAIN
#include "catch.hpp"

"""

jeventprocessor_template_h = """
#ifndef _{name}_h_
#define _{name}_h_

#include <JANA/JEventProcessor.h>

class {name} : public JEventProcessor {{
public:

    {name}();
    virtual ~{name}() = default;

    void Init() override;
    void Process(const std::shared_ptr<const JEvent>& event) override;
    void Finish() override;

}};


#endif // _{name}_h_

"""

jeventprocessor_template_cc = """
#include "{name}.h"

{name}::{name}() {{
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}}

void {name}::Init() {{
    // Open TFiles, set up TTree branches, etc
}}

void {name}::Process(const std::shared_ptr<const JEvent> &event) {{
    // Get intermediate results from Factories
    // Lock mutex
    // Do file operations 
}}

void {name}::Finish() {{
    // Close any resources
}}

"""

jeventprocessor_template_tests = """
"""


def create_jobject(name):
    filename = name + ".h"
    text = jobject_template_h.format(copyright_notice=copyright_notice, name=name)
    with open(filename, 'w') as f:
        f.write(text)


def create_jeventsource(name):

    with open(name + ".h", 'w') as f:
        text = jeventsource_template_h.format(copyright_notice=copyright_notice, name=name)
        f.write(text)

    with open(name + ".cc", 'w') as f:
        text = jeventsource_template_cc.format(copyright_notice=copyright_notice, name=name)
        f.write(text)


def create_jeventprocessor(name):
    with open(name + ".h", 'w') as f:
        text = jeventprocessor_template_h.format(copyright_notice=copyright_notice, name=name)
        f.write(text)

    with open(name + ".cc", 'w') as f:
        text = jeventprocessor_template_cc.format(copyright_notice=copyright_notice, name=name)
        f.write(text)

    with open(name + "Tests.cc", 'w') as f:
        text = jeventprocessor_template_tests.format(copyright_notice=copyright_notice, name=name)
        f.write(text)


def create_jfactory(name):
    pass


def create_plugin(name):
    import os
    os.mkdir(name)
    os.mkdir(name + "/cmake")
    os.mkdir(name + "/src")
    os.mkdir(name + "/tests")

    with open(name + "/cmake/FindJANA.cmake", 'w') as f:
        f.write(plugin_findjana_cmake)

    with open(name + "/src/CMakeLists.txt", 'w') as f:
        text = plugin_cmakelists_txt.format(name=name)
        f.write(text)

    with open(name + "/src/" + name + ".cc", 'w') as f:
        text = plugin_main.format(name=name)
        f.write(text)

    with open(name + "/src/" + name + "Processor.cc", 'w') as f:
        text = jeventprocessor_template_cc.format(name=name+"Processor")
        f.write(text)

    with open(name + "/src/" + name + "Processor.h", 'w') as f:
        text = jeventprocessor_template_h.format(name=name+"Processor")
        f.write(text)

    with open(name + "/tests/tests_main.cc", "w") as f:
        text = plugin_tests_main_cc.format(name=name)
        f.write(text)

    with open(name + "/tests/integration_tests.cc", "w") as f:
        text = plugin_integration_tests_cc.format(name=name)
        f.write(text)

    with open(name + "/tests/CMakeLists.txt", "w") as f:
        text = plugin_tests_cmakelists_txt.format(name=name)
        f.write(text)

def create_executable(name):
    pass


def create_project(name):
    pass


def print_usage():
    print("Usage: jana-generate [type] [name]")
    print("  type: JObject JEventSource JEventProcessor JFactory plugin executable project")
    print("  name: Preferably in CamelCase, e.g. EvioSource")


if __name__ == '__main__':

    if len(argv) < 3:
        print("Error: Wrong number of arguments!")
        print_usage()
        exit()

    dispatch_table = {'JObject': create_jobject,
                      'JEventSource': create_jeventsource,
                      'JEventProcessor': create_jeventprocessor,
                      'JFactory': create_jfactory,
                      'plugin': create_plugin,
                      'executable': create_executable,
                      'project': create_project
                      }

    option = argv[1]
    name = argv[2]
    if option in dispatch_table:
        dispatch_table[option](name)
    else:
        print("Error: Invalid type!")
        print_usage()
        exit()


