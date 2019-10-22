cmake_minimum_required(VERSION 3.13)
project(jana2)

set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Figure out install directory.
#   1. If CMAKE_INSTALL_PREFIX specified as an argument to CMake, use that
#   2. If JANA_HOME specified as an argument to CMake, use that
#   3. If environment variable $JANA_HOME defined, use that
#   4. Otherwise, install to /usr/local or similar

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED JANA_HOME)
        set(CMAKE_INSTALL_PREFIX ${JANA_HOME} CACHE PATH "magic incantation" FORCE)
        message(STATUS "Installing to ${JANA_HOME} (From CMake variable $JANA_HOME)")
    elseif(DEFINED ENV{JANA_HOME})
        set(CMAKE_INSTALL_PREFIX $ENV{JANA_HOME} CACHE PATH "magic incantation" FORCE)
        message(STATUS "Installing to  $ENV{JANA_HOME} (From environment variable $JANA_HOME)")
    else()
        message(STATUS "Installing to  ${CMAKE_INSTALL_PREIX} (No JANA_HOME specified, defaulting)")
    endif()
else()
    message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX} (Non-default CMAKE_INSTALL_PREFIX)")
endif()

include_directories(src/lib)   # So that everyone can find the JANA header files

add_subdirectory(src/lib/JANA)

add_subdirectory(src/examples/JExample1)
add_subdirectory(src/examples/JExample2)
add_subdirectory(src/examples/JExample7)
add_subdirectory(src/examples/event_group_example)

add_subdirectory(src/plugins/JTest)
add_subdirectory(src/plugins/streamDet)
add_subdirectory(src/plugins/janapy)
add_subdirectory(src/plugins/janarate)

add_subdirectory(src/programs/jana)
add_subdirectory(src/programs/tests)
add_subdirectory(src/programs/playground)

install(PROGRAMS scripts/mkplugin scripts/mkclass DESTINATION bin)

include(${CMAKE_SOURCE_DIR}/cmake/MakeConfig.cmake)
