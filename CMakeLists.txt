cmake_minimum_required(VERSION 3.13)
project(jana2)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")   # Define location of cmake modules

# Figure out install directory.
#   1. If specified as an argument to CMake, use that
#   2. If $JANA_HOME defined, use that
#   3. Otherwise, install to cmake build directory
#   Do NOT install to default install directory, which is /usr/local.

if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    if(DEFINED ENV{JANA_HOME})
        install(CODE "message(STATUS \"Installing to \" ENV{JANA_HOME} \"(From $JANA_HOME)\")")
        set(CMAKE_INSTALL_PREFIX ENV{JANA_HOME} CACHE PATH "magic incantation" FORCE)
    else()
        install(CODE "message(STATUS \"Installing to \" ${CMAKE_BINARY_DIR} \" (Undefined environment var $JANA_HOME)\")")
        set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR} CACHE PATH "magic incantation" FORCE)
    endif()
else()
    install(CODE "message(STATUS \"Installing to \" ${CMAKE_INSTALL_PREFIX} \" (Non-default CMAKE_INSTALL_PREFIX)\")")
endif()

include_directories(src/lib)   # So that everyone can find the JANA header files

add_subdirectory(src/lib/JANA)

add_subdirectory(src/examples/JExample1)
add_subdirectory(src/examples/JExample2)
add_subdirectory(src/examples/JExample7)

add_subdirectory(src/plugins/JTest)
add_subdirectory(src/plugins/streamDet)
add_subdirectory(src/plugins/janapy)
add_subdirectory(src/plugins/janarate)

add_subdirectory(src/programs/jana)
add_subdirectory(src/programs/tests)
add_subdirectory(src/programs/playground)

install(FILES scripts/mkplugin scripts/mkclass DESTINATION bin)


# Generate jana_config script from Python, and install to $JANA_HOME/bin

set(jana_config_file ${CMAKE_CURRENT_BINARY_DIR}/jana_config)

add_custom_command(OUTPUT ${jana_config_file}
                   DEPENDS "${CMAKE_SOURCE_DIR}/scripts/make_jana_config.py"
                   COMMAND "python"
		           ARGS "${CMAKE_SOURCE_DIR}/scripts/make_jana_config.py"
                   WORKING_DIR ${CMAKE_CURRENT_BINARY_DIR}
				   COMMENT "Generating jana_config bash script via Python"
                   VERBATIM)

# Our custom command won't even run unless attached to a target. In CMake, file != target.
# Once we've defined a throwaway target, CMake won't let us install the target directly,
# so we have to install the underlying file instead.
# See: https://samthursfield.wordpress.com/2015/11/21/cmake-dependencies-between-targets-and-files-and-custom-commands/

add_custom_target(jana_config_target ALL DEPENDS ${jana_config_file})
install(FILES ${jana_config_file} DESTINATION bin)


