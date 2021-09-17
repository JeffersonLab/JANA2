cmake_minimum_required(VERSION 3.9)
project(jana2 VERSION 2.0.4)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)   # Enable -fPIC for all targets
add_compile_options(-Wall -Wextra)
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0 -g")

option(USE_ASAN "Compile with address sanitizer" OFF)
if (${USE_ASAN})
    add_compile_options(-fsanitize=address)
#    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=address")  # TODO: Delete this once I verify its replacement works
endif()

# Set default standard to C++14, but allow user to override at command line
if(NOT "${CMAKE_CXX_STANDARD}")
  set(CMAKE_CXX_STANDARD 14)
endif()

# Expose custom cmake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# Require the user to specify CMAKE_INSTALL_PREFIX directly. DO NOT AUTOMATICALLY INSTALL to /usr/local!
# Remember that we ultimately want CMAKE_INSTALL_PREFIX=$ENV{JANA_HOME}
if(NOT CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    message(STATUS "Installing to ${CMAKE_INSTALL_PREFIX}")
else()
    message(WARNING "Missing CMAKE_INSTALL_PREFIX=$JANA_HOME => Defaulting to ./install")
    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Comment explaining this nonsense" FORCE)
endif()

#---------------------------------------------------------------------------------------
option(USE_ROOT "Compile optional targets that require ROOT" ON)
if (${USE_ROOT})
    find_package(ROOT)
    if(${ROOT_FOUND})
        message(STATUS "USE_ROOT=ON: Found ROOT at: " ${ROOT_USE_FILE})
        include(${ROOT_USE_FILE})
        add_compile_definitions(HAVE_ROOT)
        include_directories(${ROOT_INCLUDE_DIRS})
        link_libraries(${ROOT_LIBRARIES})
    else()
        message(ERROR "USE_ROOT=ON: No ROOT found (make sure CMAKE_PREFIX_PATH includes ROOT location)")
    endif()
else()
    message(STATUS "USE_ROOT=OFF: Additional ROOT support will not be included")
endif()
#---------------------------------------------------------------------------------------

include_directories(src/libraries)   # So that everyone can find the JANA header files

add_subdirectory(src/libraries/JANA)
add_subdirectory(src/examples)

add_subdirectory(src/plugins/JTest)
add_subdirectory(src/plugins/JTestRoot)
add_subdirectory(src/plugins/streamDet)
add_subdirectory(src/plugins/janarate)
add_subdirectory(src/plugins/janacontrol)

add_subdirectory(src/programs/jana)
add_subdirectory(src/programs/tests)

#---------------------------------------------------------------------------------------
option(USE_PYTHON "Compile optional python targets. This requires python-devel and python-distutils." OFF)
if (${USE_PYTHON})
    message(STATUS "USE_PYTHON=ON: Including optional python targets")
    add_subdirectory(src/python)
else()
    message(STATUS "USE_PYTHON=OFF: Skipping optional python targets")
endif()
#---------------------------------------------------------------------------------------

install(DIRECTORY scripts/ DESTINATION bin FILES_MATCHING PATTERN "jana-*.py"
        PERMISSIONS OWNER_EXECUTE OWNER_WRITE OWNER_READ GROUP_EXECUTE GROUP_READ WORLD_READ WORLD_EXECUTE)
include(${CMAKE_SOURCE_DIR}/cmake/MakeConfig.cmake)
