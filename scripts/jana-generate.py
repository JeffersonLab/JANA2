#!/usr/bin/env python3
from sys import argv
import subprocess
import sys
import os

disapatch_table = {}


def copy_from_source_dir(files_to_copy):
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


def boolify(x, name):
    if x==True or x=="True" or x=="true" or x=="1":
        return True
    elif x==False or x=="False" or x=="false" or x=="0":
        return False
    else:
        raise Exception("Argument "+name+ " must be either 'true' or 'false'")


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
    JOBJECT_PUBLIC({name})

    int x;     // Pixel coordinates centered around 0,0
    int y;     // Pixel coordinates centered around 0,0
    double E;  // Energy loss in GeV
    double t;  // Time in ms


    /// Make it convenient to construct one of these things
    {name}(int x, int y, double E, double t) : x(x), y(y), E(E), t(t) {{}};


    /// Override Summarize to tell JANA how to produce a convenient string representation for our JObject.
    /// This can be called from user code, but also lets JANA automatically inspect its own data. See the
    /// CsvWriter example. Warning: Because this is slow, it should be used for debugging and monitoring 
    /// but not inside the performance critical code paths.

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
    {name}();

    virtual ~{name}() = default;

    void Open() override;

    void Close() override;

    Result Emit(JEvent& event) override;
    
    static std::string GetDescription();

}};

template <>
double JEventSourceGeneratorT<{name}>::CheckOpenable(std::string);

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

{name}::{name}() : JEventSource() {{
    SetTypeName(NAME_OF_THIS); // Provide JANA with class name
    SetCallbackStyle(CallbackStyle::ExpertMode);
}}

void {name}::Open() {{

    /// Open is called exactly once when processing begins.
    
    /// Get any configuration parameters from the JApplication
    // GetApplication()->SetDefaultParameter("{name}:random_seed", m_seed, "Random seed");

    /// For opening a file, get the filename via:
    // std::string resource_name = GetResourceName();
    /// Open the file here!
}}

void {name}::Close() {{

    /// Close is called exactly once when processing ends. This is where you should close your files or sockets.
    /// It is important to do that here instead of in Emit() because we want everything to be cleanly closed 
    /// even when JANA is terminated via Ctrl-C or via a timeout.

}}

JEventSource::Result {name}::Emit(JEvent& event) {{

    /// Calls to GetEvent are synchronized with each other, which means they can
    /// read and write state on the JEventSource without causing race conditions.
    
    /// Configure event and run numbers
    static size_t current_event_number = 1;
    event.SetEventNumber(current_event_number++);
    event.SetRunNumber(22);

    /// Insert whatever data was read into the event
    // std::vector<Hit*> hits;
    // hits.push_back(new Hit(0,0,1.0,0));
    // event.Insert(hits);

    /// If you are reading a file of events and have reached the end, terminate the stream like this:
    /// Note that you should close any file handles or sockets in Close(), not here!
    // return Result::FailureFinished;

    /// If you are streaming events and there are no new events in the message queue,
    /// tell JANA that Emit() was temporarily unsuccessful like this:
    // return Result::FailureTryAgain;

    return Result::Success;
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

project_cmakelists_txt = """
cmake_minimum_required(VERSION 3.9)
project({name}_project)

if(NOT "${{CMAKE_CXX_STANDARD}}")
  set(CMAKE_CXX_STANDARD 17)
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${{CMAKE_CURRENT_LIST_DIR}}/cmake")

# Set install directory to $JANA_HOME
set(CMAKE_INSTALL_PREFIX $ENV{{JANA_HOME}} CACHE PATH "magic incantation" FORCE)

# Find dependencies
find_package(JANA REQUIRED)

"""


project_src_cmakelists_txt = """
add_subdirectory(libraries)
add_subdirectory(plugins)
add_subdirectory(programs)
"""


plugin_cmakelists_txt = """
{extra_find_packages}

# According to the internet, CMake authors discourage the use
# of GLOB for identifying source files. IMHO, this is due to
# the flawed use of cache files in CMake itself. Here, GLOB
# is used as the default. What this means is you can add source
# files and re-run cmake (after clearing the cache file) and
# they will be found without needing to modify this file.
# You also have the option of switching the following to "false"
# and managing the source file list manually the way they recommend.
if(true)
  # Automatically determine source file list.
  file(GLOB mysourcefiles *.cpp *.cc *.c  *.hpp *.hh *.h)
  set( {name}_PLUGIN_SOURCES ${{mysourcefiles}} )    
else()
  # Manually manage source file list
  set ({name}_PLUGIN_SOURCES
        {name}.cc
        {name}Processor.cc
        {name}Processor.h
  )
endif()

add_library({name}_plugin SHARED ${{{name}_PLUGIN_SOURCES}})

target_include_directories({name}_plugin PUBLIC  ${{CMAKE_SOURCE_DIR}} ${{JANA_INCLUDE_DIR}} {extra_includes})
target_link_libraries({name}_plugin ${{JANA_LIBRARY}} {extra_libraries})
set_target_properties({name}_plugin PROPERTIES PREFIX "" OUTPUT_NAME "{name}" SUFFIX ".so")

install(TARGETS {name}_plugin DESTINATION plugins)

file(GLOB my_headers "*.h*")
install(FILES ${{my_headers}} DESTINATION include/{name})

# For root dictionaries
file(GLOB my_pcms "${{CMAKE_CURRENT_BINARY_DIR}}/*.pcm")
install(FILES ${{my_pcms}} DESTINATION plugins)

"""


mini_plugin_cmakelists_txt = """
{extra_find_packages}

# According to the internet, CMake authors discourage the use
# of GLOB for identifying source files. IMHO, this is due to
# the flawed use of cache files in CMake itself. Here, GLOB
# is used as the default. What this means is you can add source
# files and re-run cmake (after clearing the cache file) and
# they will be found without needing to modify this file.
# You also have the option of switching the following to "false"
# and managing the source file list manually the way they recommend.
if(true)
  # Automatically determine source file list.
  file(GLOB mysourcefiles *.cpp *.cc *.c  *.hpp *.hh *.h)
  set( {name}_PLUGIN_SOURCES ${{mysourcefiles}} )    
else()
  # Manually manage source file list
  set ({name}_PLUGIN_SOURCES
        {name}.cc
  )
endif()

add_library({name}_plugin SHARED ${{{name}_PLUGIN_SOURCES}})

target_include_directories({name}_plugin PUBLIC ${{CMAKE_SOURCE_DIR}} ${{JANA_INCLUDE_DIR}} {extra_includes})
target_link_libraries({name}_plugin ${{JANA_LIB}} {extra_libraries})
set_target_properties({name}_plugin PROPERTIES PREFIX "" OUTPUT_NAME "{name}" SUFFIX ".so")

install(TARGETS {name}_plugin DESTINATION plugins)

file(GLOB my_headers "*.h*")
install(FILES ${{my_headers}} DESTINATION include/{name})

# For root dictionaries
file(GLOB my_pcms "${{CMAKE_CURRENT_BINARY_DIR}}/*.pcm")
install(FILES ${{my_pcms}} DESTINATION plugins)

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

TEST_CASE("{name}IntegrationTests") {{

    auto app = new JApplication;
    
    // Create and register components
    // app->Add(new {name}Processor);
    
    // TODO: Add (mocked) event source
    // app->Add(new MockEventSource);

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

//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
// Please add the following lines to your InitPlugin or similar routine
// in order to register this factory with the system.
//
// #include "{name}.h"
//
//     app->Add( new JFactoryGeneratorT<{name}>() );
//- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 


//------------------------
// Constructor
//------------------------
{name}::{name}(){{
    SetTag("{tag}");
}}

//------------------------
// Init
//------------------------
void {name}::Init() {{
    auto app = GetApplication();
    
    /// Acquire any parameters
    // app->GetParameter("parameter_name", m_destination);
    
    /// Acquire any services
    // m_service = app->GetService<ServiceT>();
    
    /// Set any factory flags
    // SetFactoryFlag(JFactory_Flags_t::NOT_OBJECT_OWNER);
}}

//------------------------
// ChangeRun
//------------------------
void {name}::ChangeRun(const std::shared_ptr<const JEvent> &event) {{
    /// This is automatically run before Process, when a new run number is seen
    /// Usually we update our calibration constants by asking a JService
    /// to give us the latest data for this run number
    
    auto run_nr = event->GetRunNumber();
    // m_calibration = m_service->GetCalibrationsForRun(run_nr);
}}

//------------------------
// Process
//------------------------
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
    {name}();
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
    TH1D* h1d_pt_reco = nullptr;
    TDirectory* dest_file = nullptr;
    TDirectory* dest_dir=nullptr; // Virtual subfolder inside dest_file used for this specific processor

public:
    {processor_name}();
    
    void Init() override;

    void Process(const std::shared_ptr<const JEvent>& event) override;

    void Finish() override;
}};

"""


jroot_output_processor_cc = """
#include "{processor_name}.h"

{processor_name}::{processor_name}() {{
    SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
}}
    
void {processor_name}::Init() {{
    auto app = GetApplication();
    m_lock = app->GetService<JGlobalRootLock>();

    /// Set parameters to control which JFactories you use
    app->SetDefaultParameter("tracking_alg", m_tracking_alg);

    /// Set up histograms
    m_lock->acquire_write_lock();
    
    if( dest_file == nullptr ){{
        dest_file = new TFile("{processor_name}.root", "recreate");  /// TODO: Acquire dest_file via either a JService or a JParameter
    }}
    dest_dir = dest_file->mkdir("{dir_name}"); // Create a subdir inside dest_file for these results
    h1d_pt_reco = new TH1D("pt_reco", "reco pt", 100,0,10);
    h1d_pt_reco->SetDirectory(dest_dir);
    m_lock->release_lock();
}}

void {processor_name}::Process(const std::shared_ptr<const JEvent>& event) {{

    /// Acquire any results you need for your analysis
    //auto reco_tracks = event->Get<RecoTrack>(m_tracking_alg);

    m_lock->acquire_write_lock();
    /// Inside the global root lock, update histograms
    // for (auto reco_track : reco_tracks) {{
    //    h1d_pt_reco->Fill(reco_track->p.Pt());
    // }}
    m_lock->release_lock();
}}

void {processor_name}::Finish() {{
    // TODO: If we did not create this file then we should not delete it
    dest_file->Write();
    delete dest_file;
    dest_file = nullptr;
}};

"""


mini_plugin_cc_noroot = """
#include <JANA/JEventProcessor.h>

class {name}Processor: public JEventProcessor {{
private:
    std::string m_tracking_alg = "genfit";
    std::mutex m_mutex;

public:
    {name}Processor() {{
        SetTypeName(NAME_OF_THIS); // Provide JANA with this class's name
    }}
    
    void Init() override {{
        auto app = GetApplication();

        /// Set parameters to control which JFactories you use
        app->SetDefaultParameter("tracking_alg", m_tracking_alg);
    }}

    void Process(const std::shared_ptr<const JEvent>& event) override {{

        /// Acquire any per-event results you need for your analysis
        // auto reco_tracks = event->Get<RecoTrack>(m_tracking_alg);

        /// Inside the lock, update any shared state, e.g. histograms
        std::lock_guard<std::mutex> lock(m_mutex);
        
        // for (auto reco_track : reco_tracks) {{
        //    histogram->Fill(reco_track->p.Pt());
        // }}
    }}

    void Finish() override {{
        // Write data and close resources
    }}
}};
    
extern "C" {{
    void InitPlugin(JApplication *app) {{
        InitJANAPlugin(app);
        app->Add(new {name}Processor);
    }}
}}
    
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
        dest_file = new TFile("{name}.root", "recreate");  /// TODO: Acquire dest_file via either a JService or a JParameter
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
        // TODO: If we did not create this file then we should not delete it
        dest_file->Write();
        delete dest_file;
        dest_file = nullptr;
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
    """Create a JObject code skeleton in the current directory. Requires one argument:
       name:  The name of the JObject, e.g. "RecoTrack"
    """
    filename = name + ".h"
    text = jobject_template_h.format(copyright_notice=copyright_notice, name=name)
    print(f"Creating {name}:public JObject")
    with open(filename, 'w') as f:
        f.write(text)


def create_jeventsource(name):
    """Create a JEventSource code skeleton in the current directory. Requires one argument:
       name:  The name of the JEventSource, e.g. "CsvFileSource"
    """

    with open(name + ".h", 'w') as f:
        text = jeventsource_template_h.format(copyright_notice=copyright_notice, name=name)
        f.write(text)

    with open(name + ".cc", 'w') as f:
        text = jeventsource_template_cc.format(copyright_notice=copyright_notice, name=name)
        f.write(text)


def create_jeventprocessor(name):
    """Create a JEventProcessor code skeleton in the current directory. Requires one argument:
       name:  The name of the JEventProcessor, e.g. "TrackingEfficiencyProcessor"
    """

    with open(name + ".h", 'w') as f:
        text = jeventprocessor_template_h.format(copyright_notice=copyright_notice, name=name)
        f.write(text)

    with open(name + ".cc", 'w') as f:
        text = jeventprocessor_template_cc.format(copyright_notice=copyright_notice, name=name)
        f.write(text)

    with open(name + "Tests.cc", 'w') as f:
        text = jeventprocessor_template_tests.format(copyright_notice=copyright_notice, name=name)
        f.write(text)


def create_root_eventprocessor(processor_name, dir_name):
    """Create a ROOT-aware JEventProcessor code skeleton in the current directory. Requires two positional arguments:
       [processor_name]  The name of the JEventProcessor, e.g. "TrackingEfficiencyProcessor"
       [dir_name]        The name of the virtual directory in the ROOT file where everything goes, e.g. "trk_eff"
    """
    with open(processor_name + ".h", 'w') as f:
        text = jroot_output_processor_h.format(processor_name=processor_name, dir_name=dir_name)
        f.write(text)


def build_plugin_test(plugin_name, copy_catch_hpp=True, dir="."):

    if copy_catch_hpp:
        files_to_copy = {"catch.hpp": {"sourcedir": "src/external",
                                       "installdir": "include/JANA/external",
                                       "destname": dir+"/catch.hpp"}}
        copy_from_source_dir(files_to_copy)

    cmakelists_path = dir + "/" + "CMakeLists.txt"
    with open(cmakelists_path) as f:
        text = plugin_tests_cmakelists_txt.format(name=plugin_name)
        f.write(text)

    integration_test_cc_path = dir + "/IntegrationTests.cc"
    with open(integration_test_cc_path) as f:
        text = plugin_integration_tests_cc.format(name=plugin_name)
        f.write(text)

    tests_main_path = dir + "/TestsMain.cc"
    with open(tests_main_path) as f:
        text = plugin_tests_main_cc
        f.write(text)


def build_jeventprocessor_test(processor_name, is_mini=True, copy_catch_hpp=True, dir="."):

    if copy_catch_hpp:
        files_to_copy = {"catch.hpp": {"sourcedir": "src/external",
                                       "installdir": "include/JANA/external",
                                       "destname": dir+"/catch.hpp"}}
        copy_from_source_dir(files_to_copy)

    cmakelists_path = dir + "/" + "CMakeLists.txt"
    with open(cmakelists_path) as f:
        text = plugin_tests_cmakelists_txt.format(name=name)
        f.write(text)

    if is_mini:
        integration_test_cc_path = dir + "/IntegrationTests.cc"
        with open(integration_test_cc_path) as f:
            text = plugin_integration_tests_cc.format(name=processor_name)
            f.write(text)
    else:
        tests_main_path = dir + "/TestsMain.cc"
        with open(tests_main_path) as f:
            text = plugin_tests_main_cc
            f.write(text)


def create_jeventprocessor_test(processor_name):
    """Create a JEventProcessor unit test skeleton in the current directory. Requires two arguments:
       processor_name:  The name of the JEventProcessor under test
    """
    build_jeventprocessor_test()


def create_jfactory(jobject_name, tag=''):
    """Create a JFactory code skeleton in the current directory. One required argument and one optional argument:
       jobject_name  The name of the JObject this factory creates, e.g. "RecoTrack"
       [tag]         Optional tag for the factory (defaults to empty string)
       
       n.b. This will also automatically create a JObject header in the current
       directory if the file does not already exist. You may need to delete this
       manually if you already have this class defined elsewhere in the source tree.
    """

    factory_name = f'JFactory_{jobject_name}'
    if tag != '' : factory_name += f'_{tag}'

    print(f"Creating {factory_name}:public JFactoryT<{jobject_name}>")
    with open(factory_name + ".cc", 'w') as f:
        text = jfactory_template_cc.format(name=factory_name, jobject_name=jobject_name, tag=tag)
        f.write(text)

    with open(factory_name + ".h", 'w') as f:
        text = jfactory_template_h.format(name=factory_name, jobject_name=jobject_name, tag=tag)
        f.write(text)
    
    # Check if a header for the JObject already exists. If not, create it.
    # The more likely scenario is that someone creating a factory will also
    # need to create this.
    jobject_header_filename = jobject_name + ".h"
    if not os.path.exists( jobject_header_filename ):
        create_jobject( jobject_name )


def create_jfactory_test(factory_name, jobject_name):
    with open(factory_name + "Test.cc", 'w') as f:
        text = jfactory_test_cc.format(factory_name=factory_name, jobject_name=jobject_name);
        f.write(text)


def create_executable(name):
    """Create a code skeleton for a project executable in the current directory. Requires one positional argument:
       name:  The name of the executable, e.g. "escalate" or "halld_recon"
    """
    pass


def create_plugin(name, is_standalone=True, is_mini=True, include_root=False, include_tests=False):
    """Create a code skeleton for a plugin in its own directory. Takes the following positional arguments:
          name            The name of the plugin, e.g. "trk_eff" or "TrackingEfficiency"
         [is_standalone]  Is this a new project, or are we inside the source tree of an existing CMake project? (default=True)
         [is_mini]        Reduce boilerplate and put everything in a single file? (default=True)
         [include_root]   Include a ROOT dependency and stubs for filling a ROOT histogram? (default=False)

       Example: `jana_generate.py Plugin TrackingEfficiency 1 0 0`
    """

    is_standalone = boolify(is_standalone, "is_standalone")
    is_mini = boolify(is_mini, "is_mini")
    include_root = boolify(include_root, "include_root")
    include_tests = boolify(include_tests, "include_tests")

    os.mkdir(name)
    if not is_standalone:
        with open("CMakeLists.txt", 'a') as f:
            f.write("add_subdirectory("+name+")\n")
            # If CMakeLists doesn't already exist, this will create it
            # TODO: Probably want to auto detect is_standalone based on existence of parent CMakeLists

    cmakelists = ""
    if include_root:
        extra_find_packages = "find_package(ROOT REQUIRED)"
        extra_includes = "${ROOT_INCLUDE_DIRS}"
        extra_libraries = "${ROOT_LIBRARIES}"
    else:
        extra_find_packages = ""
        extra_includes = ""
        extra_libraries = ""

    if is_standalone:
        cmakelists += project_cmakelists_txt.format(name=name)

    if not is_mini:
        with open(name + "/" + name + ".cc", 'w') as f:
            text = plugin_main.format(name=name)
            f.write(text)

        # Otherwise InitPlugin goes into the processor file

    if include_root and is_mini:
        cmakelists += mini_plugin_cmakelists_txt.format(name=name,
                                                        extra_find_packages=extra_find_packages,
                                                        extra_includes=extra_includes,
                                                        extra_libraries=extra_libraries)
        with open(name + "/" + name + ".cc", 'w') as f:
            text = mini_plugin_cc.format(name=name)
            f.write(text)

    elif include_root and not is_mini:
        with open(name + "/" + name + "Processor.h", 'w') as f:
            text = jroot_output_processor_h.format(processor_name=name+"Processor", dir_name=name)
            f.write(text)

        with open(name + "/" + name + "Processor.cc", 'w') as f:
            text = jroot_output_processor_cc.format(processor_name=name+"Processor", dir_name=name)
            f.write(text)

        cmakelists += plugin_cmakelists_txt.format(name=name,
                                                   extra_find_packages=extra_find_packages,
                                                   extra_includes=extra_includes,
                                                   extra_libraries=extra_libraries)

    elif not include_root and is_mini:
        cmakelists += mini_plugin_cmakelists_txt.format(name=name,
                                                        extra_find_packages=extra_find_packages,
                                                        extra_includes=extra_includes,
                                                        extra_libraries=extra_libraries)
        with open(name + "/" + name + ".cc", 'w') as f:
            text = mini_plugin_cc_noroot.format(name=name)
            f.write(text)

    else:
        # not include_root and not is_mini:
        cmakelists += plugin_cmakelists_txt.format(name=name,
                                                   extra_find_packages=extra_find_packages,
                                                   extra_includes=extra_includes,
                                                   extra_libraries=extra_libraries)

        with open(name + "/" + name + "Processor.cc", 'w') as f:
            text = jeventprocessor_template_cc.format(name=name+"Processor")
            f.write(text)

        with open(name + "/" + name + "Processor.h", 'w') as f:
            text = jeventprocessor_template_h.format(name=name+"Processor")
            f.write(text)

    if include_tests:
        with open(name + "/" + name + "ProcessorTest.cc", 'w') as f:
            text = jeventprocessor_template_tests.format(name=name+"Processor")
            f.write(text)

        # Copy tests CMakeLists stub
        # Copy catch2
        # Copy tests for processor

    # Write CMakeLists body
    with open(name + "/CMakeLists.txt", 'w') as f:
        f.write(cmakelists)


def create_project(name):
    """Create a code skeleton for a complete project in its own directory. Requires one argument:
       name:  The name of the project, e.g. "escalate" or "halld_recon"
    """
    os.mkdir(name)
    os.mkdir(name + "/cmake")
    os.mkdir(name + "/external")
    os.mkdir(name + "/src")
    os.mkdir(name + "/src/libraries")
    os.mkdir(name + "/src/plugins")
    os.mkdir(name + "/src/programs")
    os.mkdir(name + "/src/programs/cli")
    # os.mkdir(name + "/tests")

    with open(name + "/CMakeLists.txt", 'w') as f:
        text = project_cmakelists_txt.format(name=name)
        f.write(text)

    with open(name + "/src/CMakeLists.txt", 'w') as f:
        text = project_src_cmakelists_txt
        f.write(text)

    # with open(name + "/tests/CMakeLists.txt", 'w') as f:
    #     f.write("")

    with open(name + "/src/libraries/CMakeLists.txt", 'w') as f:
        f.write("")

    with open(name + "/src/plugins/CMakeLists.txt", 'w') as f:
        f.write("\n")

    with open(name + "/src/programs/CMakeLists.txt", 'w') as f:
        f.write("")

def print_usage():
    print("Usage: jana-generate.py [-h|--help] [type] [args...]")
    print("  type: " + ' '.join(dispatch_table.keys()))

def print_help():
    print("""
jana-generate.py : Generate skeleton code template for various JANA code constructs.
  
Usage: jana-generate.py [-h|--help] [type] [args...]

  This is used to generate a small piece of skeleton code that can then be easily
edited to implement a specific class or plugin in JANA. The type argument specifies
the type of construct to generate. Optional arguments vary depending on the type.
The type argument should be one of:
""")
    print('\n'.join(dispatch_table.keys()))

    print('\n--------------------------------------------------------------------')
    print('Details:\n')
   
    for (name,proc) in dispatch_table.items():
        print( name )
        print( proc.__doc__ )


#========================================================================================
if __name__ == '__main__':

    if sys.version_info < (3, 6):
        print('Please upgrade your Python version to 3.6.0 or higher')
        sys.exit()

    # dispatch_table must be defined here since print_usage uses it
    dispatch_table = {'JObject': create_jobject,
                      'JEventSource': create_jeventsource,
                      'JEventProcessor': create_jeventprocessor,
                      'RootEventProcessor': create_root_eventprocessor,
                      'JEventProcessorTest': create_jeventprocessor_test,
                      'JFactory': create_jfactory,
                      # 'JFactoryTest': create_jfactory_test,
                      # 'Executable': create_executable,
                      'Plugin': create_plugin,
                      'Project': create_project,
                      #'Test': run_tests
                      }

    # Print short usage statement if no arguments
    if len(argv) < 2:
        print_usage()
        exit()
    
    # Print longer help if user specifies -h or --help
    if len(argv) == 2:
        if argv[1]=='-h' or argv[1]=='--help':
            print_help()
            exit()

    # Run generator for user specified type
    option = argv[1]
    if option in dispatch_table:
        try:
            dispatch_table[option](*argv[2:])
        except TypeError:
            print('\n'+dispatch_table[option].__doc__)
    else:
        print("Error: Invalid option!")
        print_usage()
        exit()


