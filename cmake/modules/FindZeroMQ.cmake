# - Try to find ZEROMQ
# Once done this will define
# ZEROMQ_FOUND - System has ZEROMQ
# ZEROMQ_INCLUDE_DIRS - The ZEROMQ include directories
# ZEROMQ_LIBRARIES - The libraries needed to use ZEROMQ
# ZEROMQ_DEFINITIONS - Compiler switches required for using ZEROMQ
find_path (ZEROMQ_INCLUDE_DIR zmq.h)
find_library (ZEROMQ_LIBRARY NAMES zmq)
set (ZEROMQ_LIBRARIES ${ZEROMQ_LIBRARY})
set (ZEROMQ_INCLUDE_DIRS ${ZEROMQ_INCLUDE_DIR})
include (FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZEROMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args (ZEROMQ DEFAULT_MSG ZEROMQ_LIBRARY ZEROMQ_INCLUDE_DIR)
