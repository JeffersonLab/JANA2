cmake_minimum_required(VERSION 3.13)
project(jana2)

set(CMAKE_CXX_STANDARD 14)

# Enable -fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Define location of cmake modules
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# JANA,playground,tests all need threads


include_directories(src/lib)
include_directories(src/lib/JANA)
include_directories(src/plugins/JTest)
include_directories(src/programs/jana)
include_directories(src/programs/tests)


if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(CMAKE_INSTALL_PREFIX "/my/default" CACHE PATH "..." FORCE)
endif()

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
