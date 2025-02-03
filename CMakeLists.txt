cmake_minimum_required(VERSION 3.16)
cmake_policy(SET CMP0074 NEW) # find_package() uses <PackageName>_ROOT implicit hints

project(jana2 VERSION 2.3.3)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Default the C++ standard to C++17, and validate that they provided one we can use
set(CMAKE_CXX_STANDARD 17 CACHE STRING "Set the C++ standard to be used")
if(NOT CMAKE_CXX_STANDARD MATCHES "14|17|20|23")
    message(FATAL_ERROR "Unsupported C++ standard: ${CMAKE_CXX_STANDARD}")
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -g -Wall -Wextra")

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Require the user to specify CMAKE_INSTALL_PREFIX directly. DO NOT AUTOMATICALLY INSTALL to /usr/local!
# Remember that we ultimately want CMAKE_INSTALL_PREFIX=$ENV{JANA_HOME}
if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX} .")
elseif(DEFINED ENV{JANA_HOME})
    message(STATUS "Installing to $ENV{JANA_HOME} ..")
    set(CMAKE_INSTALL_PREFIX $ENV{JANA_HOME} CACHE PATH "Comment explaining this nonsense" FORCE)
else()
    message(STATUS "Missing CMAKE_INSTALL_PREFIX=$JANA_HOME => Defaulting to ${CMAKE_BINARY_DIR}/install")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Comment explaining this nonsense" FORCE)
endif()


# Add library directories to RPATH for all targets
set( CMAKE_SKIP_BUILD_RPATH FALSE )
set( CMAKE_BUILD_WITH_INSTALL_RPATH FALSE )
set( CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${CMAKE_INSTALL_PREFIX}/lib/JANA/plugins" )
set( CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE )

# Generate a compilation database, e.g. for IDE autocompletion
set( CMAKE_EXPORT_COMPILE_COMMANDS TRUE )

# Useful for debugging. Copied from:
# https://stackoverflow.com/questions/9298278/cmake-print-out-all-accessible-variables-in-a-script
function(dump_cmake_variables)
    get_cmake_property(_variableNames VARIABLES)
    list (SORT _variableNames)
    foreach (_variableName ${_variableNames})
        if (ARGV0)
            unset(MATCHED)
            string(REGEX MATCH ${ARGV0} MATCHED ${_variableName})
            if (NOT MATCHED)
                continue()
            endif()
        endif()
        message(STATUS "${_variableName}=${${_variableName}}")
    endforeach()
endfunction()


#----------------------
# Library dependencies
#----------------------

option(USE_ROOT "Include ROOT dependency." OFF)
option(USE_ZEROMQ "Include ZeroMQ dependency. (Needed for examples/StreamingExample, plugins/streamDet, plugins/janacontrol), " OFF)
option(USE_XERCES "Include XercesC 3 dependency. (Needed for JGeometryXML)" OFF)
option(USE_PYTHON "Include Python dependency. This requires python-devel and python-distutils." OFF)
option(USE_ASAN "Compile with address sanitizer" OFF)
option(USE_TSAN "Compile with thread sanitizer" OFF)
option(USE_CUDA "Compile CUDA-involved examples (Needed for examples/SubeventCUDAExample)." OFF)
option(USE_PODIO "Compile with PODIO support" OFF)
option(BUILD_SHARED_LIBS "Build into both shared and static libs." ON)

if (${USE_PODIO})
    find_package(podio REQUIRED)
    set(JANA2_HAVE_PODIO 1)
    set(USE_ROOT ON)
    include_directories(SYSTEM ${podio_INCLUDE_DIR})
else()
    set(JANA2_HAVE_PODIO 0)
endif()


if (${USE_ROOT})
    if((NOT DEFINED ROOT_DIR) AND (DEFINED ENV{ROOTSYS}))
        set(ROOT_DIR $ENV{ROOTSYS}/cmake)
    endif()
    find_package(ROOT REQUIRED)
    set(JANA2_HAVE_ROOT 1)
    include_directories(${ROOT_INCLUDE_DIRS})
    link_libraries(${ROOT_LIBRARIES})
    execute_process(
        COMMAND ${ROOT_BINDIR}/root-config --features
        OUTPUT_VARIABLE ROOT_FEATURES
    )
    if(NOT ${ROOT_FEATURES} MATCHES .*cxx${CMAKE_CXX_STANDARD}.*)
        message(STATUS "root-config --features: ${ROOT_FEATURES}.")
        message(FATAL_ERROR "ROOT was not compiled against C++${CMAKE_CXX_STANDARD}. "
            "Specify the C++ standard used to compile ROOT with e.g. -DCMAKE_CXX_STANDARD=17. "
            "Check the root-config output above for cxx flags.")
    endif()
else()
    set(JANA2_HAVE_ROOT 0)
endif()

if (${USE_ZEROMQ})
    find_package(ZeroMQ REQUIRED)
endif()

if (${USE_XERCES})
    if((NOT DEFINED XercesC_DIR) AND (DEFINED ENV{XERCESCROOT}))
        set(XercesC_DIR $ENV{XERCESCROOT})
    endif()
    find_package(XercesC REQUIRED)
    set(JANA2_HAVE_XERCES 1)
    include_directories(${XercesC_INCLUDE_DIRS})
    link_libraries(${XercesC_LIBRARIES})
else()
    set(JANA2_HAVE_XERCES 0)
endif()

if (${USE_ASAN})
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

if (${USE_TSAN})
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif()

if (${USE_CUDA})
    find_package(CUDA REQUIRED)
endif()


#---------
# Report back to the user what we've discovered
#---------

message(STATUS "-----------------------")
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ standard is ${CMAKE_CXX_STANDARD}")
message(STATUS "Installation directory is ${CMAKE_INSTALL_PREFIX}")
if (${USE_ROOT})
    message(STATUS "USE_ROOT    On  --> " ${ROOT_DIR})
else()
    message(STATUS "USE_ROOT    Off")
endif()
if (${USE_ZEROMQ})
    message(STATUS "USE_ZEROMQ  On  --> " ${ZeroMQ_DIR})
else()
    message(STATUS "USE_ZEROMQ  Off")
endif()
if (${USE_XERCES})
    message(STATUS "USE_XERCES  On  --> " ${XercesC_DIR})
else()
    message(STATUS "USE_XERCES  Off")
endif()
if (${USE_PYTHON})
    message(STATUS "USE_PYTHON  On")
else()
    message(STATUS "USE_PYTHON  Off")
endif()
if (${USE_ASAN})
    message(STATUS "USE_ASAN    On")
else()
    message(STATUS "USE_ASAN    Off")
endif()
if (${USE_TSAN})
    message(STATUS "USE_TSAN    On")
else()
    message(STATUS "USE_TSAN    Off")
endif()
if (${USE_CUDA})
    message(STATUS "USE_CUDA    On")
else()
    message(STATUS "USE_CUDA    Off")
endif()
if (${USE_PODIO})
    message(STATUS "USE_PODIO   On  --> " ${podio_DIR})
else()
    message(STATUS "USE_PODIO   Off")
endif()
if (${BUILD_SHARED_LIBS})
    message(STATUS "BUILD_SHARED_LIBS    On")
else()
    message(STATUS "BUILD_SHARED_LIBS    Off")
endif()
message(STATUS "-----------------------")

#---------
# Targets
#---------

include_directories(src/libraries)   # So that everyone can find the JANA header files
include_directories(${CMAKE_CURRENT_BINARY_DIR}/src/libraries)   # So that everyone can find JVersion.h

# This is needed on macos to allow plugins to link without resolving all JANA symbols until runtime
if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    add_link_options(-undefined dynamic_lookup)
endif()

include(CTest)
include(cmake/AddJanaPlugin.cmake)
include(cmake/AddJanaTest.cmake)

add_subdirectory(src/external)
add_subdirectory(src/libraries/JANA)
add_subdirectory(src/examples)
add_subdirectory(src/plugins)
add_subdirectory(src/programs/jana)
add_subdirectory(src/programs/unit_tests)
add_subdirectory(src/programs/perf_tests)
add_subdirectory(src/python)

#---------------------------------------------------------------------------------------

install(DIRECTORY scripts/ DESTINATION bin FILES_MATCHING PATTERN "jana-*.py"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)

    install(FILES "scripts/jana-status.sh" RENAME "jana-status" DESTINATION "bin"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)

include(${CMAKE_SOURCE_DIR}/cmake/MakeConfig.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/MakeJanaThis.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/MakeJVersionH.cmake)
