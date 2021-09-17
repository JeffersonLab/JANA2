
# Copyright 2020, Jefferson Science Associates, LLC.
# Subject to the terms in the LICENSE file found in the top-level directory.

#[=======================================================================[.rst:
FindXercesC
--------

Finds the XercesC library.

Imported Targets
^^^^^^^^^^^^^^^^

This module provides the following imported targets, if found:

``XercesC::XercesC``
  The XercesC library

Result Variables
^^^^^^^^^^^^^^^^

This will define the following variables:

``XercesC_FOUND``
  True if the system has the XercesC library.
``XercesC_VERSION``
  The version of the XercesC library which was found.
``XercesC_INCLUDE_DIRS``
  Include directories needed to use XercesC.
``XercesC_LIBRARIES``
  Libraries needed to link to XercesC.

#]=======================================================================]


if (DEFINED ENV{XERCESCROOT})
    set(XercesC_DIR $ENV{XERCESCROOT})
    message(STATUS "Using XercesC_DIR = ${XercesC_DIR} (From XERCESCROOT env variable)")
else()
    message(WARNING "$XERCESCROOT env variable not set")
endif()

set(XercesC_VERSION 3.1.4)

find_path(XercesC_INCLUDE_DIR
        NAMES "xercesc/parsers/DOMLSParserImpl.hpp"
        PATHS ${XercesC_DIR}/include
        )

find_library(XercesC_LIBRARY
        NAMES "xerces-c"
        PATHS ${XercesC_DIR}/lib
        )

set(XercesC_LIBRARIES ${XercesC_LIBRARY})
set(XercesC_INCLUDE_DIRS ${XercesC_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XercesC
        FOUND_VAR XercesC_FOUND
        VERSION_VAR XercesC_VERSION
        REQUIRED_VARS XercesC_DIR XercesC_INCLUDE_DIR XercesC_LIBRARY
        )