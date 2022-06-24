# - Try to find ZeroMQ
# Once done this will define
# ZeroMQ_FOUND - System has ZeroMQ
# ZeroMQ_INCLUDE_DIRS - The ZeroMQ include directories
# ZeroMQ_LIBRARIES - The libraries needed to use ZeroMQ
# ZeroMQ_DEFINITIONS - Compiler switches required for using ZeroMQ

find_path (ZeroMQ_INCLUDE_DIR zmq.h)
find_library (ZeroMQ_LIBRARY NAMES zmq)
set (ZeroMQ_LIBRARIES ${ZeroMQ_LIBRARY})
set (ZeroMQ_INCLUDE_DIRS ${ZeroMQ_INCLUDE_DIR})

# Use parent directory name of the include directory to
# indicate where we are getting this from. This is
# informational only for when the top-level file
# prints "USE_ZEROMQ  On  -->"
# n.b. it is possible the library and include directories
# don't point to the same version!
get_filename_component(ZeroMQ_DIR ${ZeroMQ_INCLUDE_DIR} DIRECTORY)

# For debugging
#message(STATUS "ZeroMQ_INCLUDE_DIR=${ZeroMQ_INCLUDE_DIR}")
#message(STATUS "ZeroMQ_LIBRARY=${ZeroMQ_LIBRARY}")
#message(STATUS "ZeroMQ_DIR=${ZeroMQ_DIR}")

# handle the QUIETLY and REQUIRED arguments and set ZeroMQ_FOUND to TRUE
# if all listed variables are TRUE

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZeroMQ
        FOUND_VAR ZeroMQ_FOUND
        VERSION_VAR ZeroMQ_VERSION
        REQUIRED_VARS ZeroMQ_DIR ZeroMQ_INCLUDE_DIRS ZeroMQ_LIBRARIES
        )
