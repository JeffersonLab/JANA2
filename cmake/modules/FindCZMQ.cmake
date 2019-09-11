# - Try to find CZMQ
# Once done this will define
# CZMQ_FOUND - System has CZMQ
# CZMQ_INCLUDE_DIRS - The CZMQ include directories
# CZMQ_LIBRARIES - The libraries needed to use CZMQ
# CZMQ_DEFINITIONS - Compiler switches required for using CZMQ
find_path (CZMQ_INCLUDE_DIR zmq.h)
find_library (CZMQ_LIBRARY NAMES zmq)
set (CZMQ_LIBRARIES ${CZMQ_LIBRARY})
set (CZMQ_INCLUDE_DIRS ${CZMQ_INCLUDE_DIR})
include (FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args (CZMQ DEFAULT_MSG CZMQ_LIBRARY CZMQ_INCLUDE_DIR)