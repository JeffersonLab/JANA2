cmake_minimum_required(VERSION 3.13)
project(jana2 VERSION 2.0.1)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Figure out install directory.
#   1. If JANA_HOME specified as an argument to CMake, use that
#   2. If CMAKE_INSTALL_PREFIX specified as an argument to CMake, use that
#   3. Otherwise, require user to specify JANA_HOME. DO NOT INSTALL TO /usr/local!!!

if(DEFINED JANA_HOME)
    set(CMAKE_INSTALL_PREFIX ${JANA_HOME} CACHE PATH "magic incantation" FORCE)
    message(STATUS "Installing to ${JANA_HOME} (From CMake variable $JANA_HOME)")

elseif(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX} (Non-default CMAKE_INSTALL_PREFIX)")

else()
    message(FATAL_ERROR "Please provide CMAKE_INSTALL_PREFIX or JANA_HOME (as a CMake variable, not an envvar)")
endif()

include_directories(src/lib)   # So that everyone can find the JANA header files

add_subdirectory(src/lib/JANA)
add_subdirectory(src/examples)

add_subdirectory(src/plugins/JTest)
add_subdirectory(src/plugins/streamDet)
add_subdirectory(src/plugins/janapy)
add_subdirectory(src/plugins/janarate)
add_subdirectory(src/plugins/janacontrol)

add_subdirectory(src/programs/jana)
add_subdirectory(src/programs/tests)
add_subdirectory(src/programs/playground)

install(PROGRAMS scripts/jana-generate.py scripts/jana-upgrade.py DESTINATION bin)

include(${CMAKE_SOURCE_DIR}/cmake/MakeConfig.cmake)
