cmake_minimum_required(VERSION 3.9)
project(jana2 VERSION 2.0.4)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Set default standard to C++14, but allow user to override at command line
if(NOT "${CMAKE_CXX_STANDARD}")
    set(CMAKE_CXX_STANDARD 14)
endif()

string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -g -Wall -Wextra")

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Require the user to specify CMAKE_INSTALL_PREFIX directly. DO NOT AUTOMATICALLY INSTALL to /usr/local!
# Remember that we ultimately want CMAKE_INSTALL_PREFIX=$ENV{JANA_HOME}
if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
#        message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")
elseif(DEFINED ENV{JANA_HOME})
    set(CMAKE_INSTALL_PREFIX $ENV{JANA_HOME} CACHE PATH "Comment explaining this nonsense" FORCE)
else()
    #    message(WARNING "Missing CMAKE_INSTALL_PREFIX=$JANA_HOME => Defaulting to ./install")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Comment explaining this nonsense" FORCE)
endif()


#----------------------
# Library dependencies
#----------------------

option(USE_ROOT "Include ROOT dependency." OFF)
option(USE_ZEROMQ "Include ZeroMQ dependency. (Needed for examples/StreamingExample, programs/StreamDet, plugins/janacontrol), " OFF)
option(USE_XERCES "Include XercesC 3 dependency. (Needed for JGeometryXML)" OFF)
option(USE_PYTHON "Include Python dependency. This requires python-devel and python-distutils." OFF)
option(USE_ASAN "Compile with address sanitizer" OFF)
option(USE_TSAN "Compile with thread sanitizer" OFF)


if (${USE_ROOT})
    if((NOT DEFINED ROOT_DIR) AND (DEFINED ENV{ROOTSYS}))
        set(ROOT_DIR $ENV{ROOTSYS}/cmake)
    endif()
    find_package(ROOT REQUIRED)
    include(${ROOT_USE_FILE})
    add_compile_definitions(HAVE_ROOT)
    include_directories(${ROOT_INCLUDE_DIRS})
    link_libraries(${ROOT_LIBRARIES})
endif()

if (${USE_ZEROMQ})
    find_package(ZeroMQ REQUIRED)
endif()

if (${USE_XERCES})
    if((NOT DEFINED XercesC_DIR) AND (DEFINED ENV{XERCESCROOT}))
        set(XercesC_DIR $ENV{XERCESCROOT})
    endif()
    find_package(XercesC REQUIRED)
    add_compile_definitions(HAVE_XERCES)
    add_compile_definitions(XERCES3)
    include_directories(${XercesC_INCLUDE_DIRS})
    link_libraries(${XercesC_LIBRARIES})
endif()

if (${USE_ASAN})
    add_compile_options(-fsanitize=address -g -O2)
    add_link_options(-fsanitize=address)
endif()

if (${USE_TSAN})
    add_compile_options(-fsanitize=thread -g -O2)
    add_link_options(-fsanitize=thread)
endif()

#---------
# Report back to the user what we've discovered
#---------

message(STATUS "-----------------------")
message(STATUS "Build type is ${CMAKE_BUILD_TYPE}")
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
message(STATUS "-----------------------")

#---------
# Targets
#---------

include_directories(src/libraries)   # So that everyone can find the JANA header files

add_subdirectory(src/libraries/JANA)
add_subdirectory(src/examples)
add_subdirectory(src/plugins)
add_subdirectory(src/programs/jana)
add_subdirectory(src/programs/tests)
add_subdirectory(src/programs/perf_tests)

add_subdirectory(src/python)

#---------------------------------------------------------------------------------------

install(DIRECTORY scripts/ DESTINATION bin FILES_MATCHING PATTERN "jana-*.py"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
include(${CMAKE_SOURCE_DIR}/cmake/MakeConfig.cmake)
