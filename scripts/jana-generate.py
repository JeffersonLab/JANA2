#!/usr/bin/env python3
from sys import argv
import subprocess
import sys
import os

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

#ifndef _{name}_h_
#define  _{name}_h_

#include <JANA/JEventSource.h>
#include <JANA/JEventSourceGeneratorT.h>

class {name} : public JEventSource {{

    /// Add member variables here

public:
    {name}(std::string resource_name, JApplication* app);

    virtual ~{name}() = default;

    void Open() override;

    void GetEvent(std::shared_ptr<JEvent>) override;
    
    static std::string GetDescription();

}};

template <>
double JEventSourceGeneratorT<RandomSource>::CheckOpenable(std::string);

#endif // _{name}_h_

"""

jeventsource_template_cc = """

#include "{name}.h"

#include <JANA/JApplication.h>
#include <JANA/JEvent.h>

/// Include headers to any JObjects you wish to associate with each event
// #include "Hit.h"

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
    static size_t current_event_number = 1;
    event->SetEventNumber(current_event_number++);
    event->SetRunNumber(22);

    /// Insert whatever data was read into the event
    // std::vector<Hit*> hits;
    // hits.push_back(new Hit(0,0,1.0,0));
    // event->Insert(hits);

    /// If you are reading a file of events and have reached the end, terminate the stream like this:
    // // Close file pointer!
    // throw RETURN_STATUS::kNO_MORE_EVENTS;

    /// If you are streaming events and there are no new events in the message queue,
    /// tell JANA that GetEvent() was temporarily unsuccessful like this:
    // throw RETURN_STATUS::kBUSY;
}}

std::string {name}::GetDescription() {{

    /// GetDescription() helps JANA explain to the user what is going on
    return "";
}}


template <>
double JEventSourceGeneratorT<{name}>::CheckOpenable(std::string resource_name) {{

    /// CheckOpenable() decides how confident we are that this EventSource can handle this resource.
    ///    0.0        -> 'Cannot handle'
    ///    (0.0, 1.0] -> 'Can handle, with this confidence level'
    
    /// To determine confidence level, feel free to open up the file and check for magic bytes or metadata.
    /// Returning a confidence <- {{0.0, 1.0}} is perfectly OK!
    
    return (resource_name == "{name}") ? 1.0 : 0.0;
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

plugin_root_cmakelists_txt = """
cmake_minimum_required(VERSION 3.13)
project({name}_plugin_project)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${{CMAKE_CURRENT_LIST_DIR}}/cmake")

# Set install directory to $JANA_HOME
set(CMAKE_INSTALL_PREFIX $ENV{{JANA_HOME}} CACHE PATH "magic incantation" FORCE)

add_subdirectory(src)
add_subdirectory(tests)

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
        TestsMain.cc
        IntegrationTests.cc
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
    REQUIRE(app->GetNEventsProcessed() == 10);

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

    // Shared state (e.g. histograms, TTrees, TFiles) live
    std::mutex m_mutex;
    
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
#include <JANA/JLogger.h>

{name}::{name}() {{
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}}

void {name}::Init() {{
    LOG << "{name}::Init" << LOG_END;
    // Open TFiles, set up TTree branches, etc
}}

void {name}::Process(const std::shared_ptr<const JEvent> &event) {{
    LOG << "{name}::Process, Event #" << event->GetEventNumber() << LOG_END;
    
    /// Do everything we can in parallel
    /// Warning: We are only allowed to use local variables and `event` here
    //auto hits = event->Get<Hit>();

    /// Lock mutex
    std::lock_guard<std::mutex>lock(m_mutex);

    /// Do the rest sequentially
    /// Now we are free to access shared state such as m_heatmap
    //for (const Hit* hit : hits) {{
        /// Update shared state
    //}}
}}

void {name}::Finish() {{
    // Close any resources
    LOG << "{name}::Finish" << LOG_END;
}}

"""

jeventprocessor_template_tests = """
"""

jfactory_template_cc = """
#include "{name}.h"

#include <JANA/JEvent.h>

void {name}::Init() {{
    auto app = GetApplication();
    
    /// Acquire any parameters
    // app->GetParameter("parameter_name", m_destination);
    
    /// Acquire any services
    // m_service = app->GetService<ServiceT>();
    
    /// Set any factory flags
    // SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER);
}}

void {name}::ChangeRun(const std::shared_ptr<const JEvent> &event) {{
    /// This is automatically run before Process, when a new run number is seen
    /// Usually we update our calibration constants by asking a JService
    /// to give us the latest data for this run number
    
    auto run_nr = event->GetRunNumber();
    // m_calibration = m_service->GetCalibrationsForRun(run_nr);
}}

void {name}::Process(const std::shared_ptr<const JEvent> &event) {{

    /// JFactories are local to a thread, so we are free to access and modify
    /// member variables here. However, be aware that events are _scattered_ to
    /// different JFactory instances, not _broadcast_: this means that JFactory 
    /// instances only see _some_ of the events. 
    
    /// Acquire inputs (This may recursively call other JFactories)
    // auto inputs = event->Get<...>();
    
    /// Do some computation
    
    /// Publish outputs
    // std::vector<{jobject_name}*> results;
    // results.push_back(new {jobject_name}(...));
    // Set(results);
}}
"""

jfactory_template_h = """
#ifndef _{name}_h_
#define _{name}_h_

#include <JANA/JFactoryT.h>

#include "{jobject_name}.h"

class {name} : public JFactoryT<{jobject_name}> {{

    // Insert any member variables here

public:
    {name}() : JFactoryT<{jobject_name}>(NAME_OF_THIS) {{}};
    void Init() override;
    void ChangeRun(const std::shared_ptr<const JEvent> &event) override;
    void Process(const std::shared_ptr<const JEvent> &event) override;

}};

#endif // _{name}_h_
"""


jroot_output_processor_h = """
#include <JANA/JEventProcessor.h>
#include <JANA/Services/JGlobalRootLock.h>
#include <TH1D.h>
#include <TFile.h>

class {processor_name}: public JEventProcessor {{
private:
    std::string m_tracking_alg = "genfit";
    std::shared_ptr<JGlobalRootLock> m_lock;
    TH1D* h1d_pt_reco;
    TFile* dest_file;
    TDirectory* dest_dir; // Virtual subfolder inside dest_file used for this specific processor

public:
    {processor_name}() {{
        SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    }}
    
    void Init() override {{
        auto app = GetApplication();
        m_lock = app->GetService<JGlobalRootLock>();

        /// Set parameters to control which JFactories you use
        app->SetDefaultParameter("tracking_alg", m_tracking_alg);

        /// Set up histograms
        m_lock->acquire_write_lock();
        //dest_file = ... /// TODO: Acquire dest_file via either a JService or a JParameter
        dest_dir = dest_file->mkdir("{dir_name}"); // Create a subdir inside dest_file for these results
        dest_file->cd();
        h1d_pt_reco = new TH1D("pt_reco", "reco pt", 100,0,10);
        h1d_pt_reco->SetDirectory(dest_dir);
        m_lock->release_lock();
    }}

    void Process(const std::shared_ptr<const JEvent>& event) override {{

        /// Acquire any results you need for your analysis
        //auto reco_tracks = event->Get<RecoTrack>(m_tracking_alg);

        m_lock->acquire_write_lock();
        /// Inside the global root lock, update histograms
        // for (auto reco_track : reco_tracks) {{
        //    h1d_pt_reco->Fill(reco_track->p.Pt());
        // }}
        m_lock->release_lock();
    }}

    void Finish() override {{
        // Close TFile (unless shared)
    }};
}};

"""

mini_plugin_cmakelists_txt = """

add_library({name}_plugin SHARED {name}.cc)

find_package(JANA REQUIRED)
find_package(ROOT)

target_include_directories({name}_plugin PUBLIC ${{JANA_INCLUDE_DIR}} ${{ROOT_INCLUDE_DIRS}})
target_link_libraries({name}_plugin ${{JANA_LIB}} ${{ROOT_LIBRARIES}})
set_target_properties({name}_plugin PROPERTIES PREFIX "" OUTPUT_NAME "{name}" SUFFIX ".so")
install(TARGETS {name}_plugin DESTINATION plugins)

"""

mini_plugin_cc = """
#include <JANA/JEventProcessor.h>
#include <JANA/Services/JGlobalRootLock.h>
#include <TH1D.h>
#include <TFile.h>

class {name}Processor: public JEventProcessor {{
private:
    std::string m_tracking_alg = "genfit";
    std::shared_ptr<JGlobalRootLock> m_lock;
    TFile* dest_file;
    TDirectory* dest_dir; // Virtual subfolder inside dest_file used for this specific processor
    
    /// Declare histograms here
    TH1D* h1d_pt_reco;

public:
    {name}Processor() {{
        SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    }}
    
    void Init() override {{
        auto app = GetApplication();
        m_lock = app->GetService<JGlobalRootLock>();

        /// Set parameters to control which JFactories you use
        app->SetDefaultParameter("tracking_alg", m_tracking_alg);

        /// Set up histograms
        m_lock->acquire_write_lock();
        //dest_file = ... /// TODO: Acquire dest_file via either a JService or a JParameter
        dest_dir = dest_file->mkdir("{name}"); // Create a subdir inside dest_file for these results
        dest_file->cd();
        h1d_pt_reco = new TH1D("pt_reco", "reco pt", 100,0,10);
        h1d_pt_reco->SetDirectory(dest_dir);
        m_lock->release_lock();
    }}

    void Process(const std::shared_ptr<const JEvent>& event) override {{

        /// Acquire any results you need for your analysis
        //auto reco_tracks = event->Get<RecoTrack>(m_tracking_alg);

        m_lock->acquire_write_lock();
        /// Inside the global root lock, update histograms
        // for (auto reco_track : reco_tracks) {{
        //    h1d_pt_reco->Fill(reco_track->p.Pt());
        // }}
        m_lock->release_lock();
    }}

    void Finish() override {{
        // Close TFile (unless shared)
    }}
}};
    
extern "C" {{
    void InitPlugin(JApplication *app) {{
        InitJANAPlugin(app);
        app->Add(new {name}Processor);
    }}
}}
    
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

    #with open(name + "Tests.cc", 'w') as f:
    #    text = jeventprocessor_template_tests.format(copyright_notice=copyright_notice, name=name)
    #    f.write(text)


def create_jfactory(name, jobject_name):

    print(f"Creating {name}:public JFactoryT<{jobject_name}>")
    with open(name + ".cc", 'w') as f:
        text = jfactory_template_cc.format(name=name, jobject_name=jobject_name)
        f.write(text)

    with open(name + ".h", 'w') as f:
        text = jfactory_template_h.format(name=name, jobject_name=jobject_name)
        f.write(text)


def create_plugin(name):
    os.mkdir(name)
    os.mkdir(name + "/cmake")
    os.mkdir(name + "/src")
    os.mkdir(name + "/tests")

    # For files that are copied from a JANA source directory we need to know
    # that directory. Preference is given to a path derived from the location
    # of the currently executing script since that is most the likely to
    # succeed and most likely what the user is intending.
    # This is complicated by the fact that someone could be running this script
    # from the install directory or from the source directory OR from a copy
    # they placed somewhere else. For this last case we must rely on the
    # JANA_HOME environment variable.
    #
    # This is also complicated by the relative path to files (e.g. catch.hpp)
    # being different in the JANA source directory tree than where it is
    # installed. Thus, in order to handle all cases, we need to make a list of
    # the source files along with various relative paths. We do this using a
    # dictionary where each key is the source filename and the value is itself
    # a dictionary containing the relative paths is the source and install
    # directories as well as a destination name for the file (in case we ever
    # add one that needs to be renamed).
    files_to_copy = {}
    files_to_copy["catch.hpp"] = {"sourcedir":"src/programs/tests", "installdir":"include/external", "destname":name+"/tests/catch.hpp"}

    jana_scripts_dir = os.path.dirname(os.path.realpath(__file__)) # Directory hold current script
    jana_dir = os.path.dirname(jana_scripts_dir)                   # Parent directory of above dir

    # Guess whether this script is being run from install or source directory
    script_in_source  = (os.path.basename(jana_scripts_dir) == "scripts")
    script_in_install = (os.path.basename(jana_scripts_dir) == "bin")

    # Check for files relative to this script
    Nfile_in_source  = sum([1 for f,d in files_to_copy.items() if os.path.exists(jana_dir+"/"+d["sourcedir" ]+"/"+f)])
    Nfile_in_install = sum([1 for f,d in files_to_copy.items() if os.path.exists(jana_dir+"/"+d["installdir"]+"/"+f)])
    all_files_in_source  = (Nfile_in_source  == len(files_to_copy))
    all_files_in_install = (Nfile_in_install == len(files_to_copy))

    # This more complicated than it should be, but is done this way to try and
    # handle all ways a user may have setup their install and source dirs.
    Nerrs = 0
    use_install = script_in_install and all_files_in_install
    use_source  = script_in_source  and all_files_in_source
    if not (use_install or use_source):
        use_install = script_in_source  and all_files_in_install
        use_source  = script_in_install and all_files_in_source

    # Make a new entry in each file's dictionary of full path to actual source file
    if use_install:
        for f,d in files_to_copy.items():
            files_to_copy[f]["sourcefile"] = jana_dir+"/"+d["installdir"]+"/"+f
    if use_source:
        for f,d in files_to_copy.items():
            files_to_copy[f]["sourcefile"] = jana_dir+"/"+d["sourcedir"]+"/"+f
    else:
        # We only get here if neither the install nor source directory contain
        # all of the files we need to install (or they don't exist). Try JANA_HOME
        jana_home = os.getenv("JANA_HOME")
        if jana_home is None:
            print("ERROR: This script does not look like it is being run from a known")
            print("       location and JANA_HOME is also not set. Please set your")
            print("       JANA_HOME environment variable to a valid JANA installation.")
            sys.exit(-1)
        for f,d in files_to_copy.items():
            files_to_copy[f]["sourcefile"] = jana_home +"/"+d["installdir"]+"/"+f

    # OK, finally we can copy the files
    Nerrs = 0
    for f,d in files_to_copy.items():
        try:
            cmd = ["cp", d["sourcefile"], d["destname"]]
            print(" ".join(cmd))
            subprocess.check_call(cmd)
        except subprocess.CalledProcessError as cpe:
            print("ERROR: Copy failed of " + d["sourcefile"] + " -> " + d["destname"])
            Nerrs += 1
    if Nerrs > 0:
        print("")
        print("Errors encountered while copying files to plugin directory. Please")
        print("check your permissions, your JANA_HOME environment variable, and")
        print("you JANA installation. Continuing now though this may leave you with")
        print("a broken plugin.")

    # Write all files defined in this script
    with open(name + "/cmake/FindJANA.cmake", 'w') as f:
        f.write(plugin_findjana_cmake)

    with open(name + "/CMakeLists.txt", 'w') as f:
        text = plugin_root_cmakelists_txt.format(name=name)
        f.write(text)

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

    with open(name + "/tests/TestsMain.cc", "w") as f:
        text = plugin_tests_main_cc.format(name=name)
        f.write(text)

    with open(name + "/tests/IntegrationTests.cc", "w") as f:
        text = plugin_integration_tests_cc.format(name=name)
        f.write(text)

    with open(name + "/tests/CMakeLists.txt", "w") as f:
        text = plugin_tests_cmakelists_txt.format(name=name)
        f.write(text)


def create_project_plugin(name):
    os.mkdir(name)

    with open(name + "/CMakeLists.txt", 'w') as f:
        text = plugin_cmakelists_txt.format(name=name)
        f.write(text)

    with open(name + "/" + name + ".cc", 'w') as f:
        text = plugin_main.format(name=name)
        f.write(text)

    with open(name + "/" + name + "Processor.cc", 'w') as f:
        text = jeventprocessor_template_cc.format(name=name+"Processor")
        f.write(text)

    with open(name + "/" + name + "Processor.h", 'w') as f:
        text = jeventprocessor_template_h.format(name=name+"Processor")
        f.write(text)

    with open(name + "/" + name + "ProcessorTest.cc", 'w') as f:
        text = jeventprocessor_template_tests.format(name=name+"Processor")
        f.write(text)

def create_executable(name):
    pass


def create_project(name):
    os.mkdir(name)
    os.mkdir(name + "/cmake")
    os.mkdir(name + "/external")
    os.mkdir(name + "/src")
    os.mkdir(name + "/src/libraries")
    os.mkdir(name + "/src/plugins")
    os.mkdir(name + "/src/programs")
    os.mkdir(name + "/src/programs/cli")
    os.mkdir(name + "/tests")

def create_root_eventprocessor(processor_name, dir_name):
    with open(processor_name + ".h", 'w') as f:
        text = jroot_output_processor_h.format(processor_name=processor_name, dir_name=dir_name)
        f.write(text)


def create_mini_plugin(name):
    os.mkdir(name)
    with open(name + "/CMakeLists.txt", 'w') as f:
        text = mini_plugin_cmakelists_txt.format(name=name)
        f.write(text)

    with open(name + "/" + name + ".cc", 'w') as f:
        text = mini_plugin_cc.format(name=name)
        f.write(text)


def print_usage():
    print("Usage: jana-generate [type] [name]")
    print("  type: JObject JEventSource JEventProcessor RootEventProcessor JFactory StandalonePlugin ProjectPlugin MiniPlugin Executable Project")
    print("  name: Preferably in CamelCase, e.g. EvioSource")


if __name__ == '__main__':

    if len(argv) < 3:
        print("Error: Wrong number of arguments!")
        print_usage()
        exit()

    dispatch_table = {'JObject': create_jobject,
                      'JEventSource': create_jeventsource,
                      'JEventProcessor': create_jeventprocessor,
                      'RootEventProcessor': create_root_eventprocessor,
                      'JFactory': create_jfactory,
                      'StandalonePlugin': create_plugin,
                      'ProjectPlugin': create_project_plugin,
                      'MiniPlugin': create_mini_plugin,
                      'Executable': create_executable,
                      'Project': create_project,
                      }

    option = argv[1]
    if option in dispatch_table:
        dispatch_table[option](*argv[2:])
    else:
        print("Error: Invalid type!")
        print_usage()
        exit()


