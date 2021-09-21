#
# This is used in the generation of the files:
#   jana-config
#   jana_config.h
#   jana-this.sh
#   jana-this.csh
#
# The primary role of this file is to set cmake variables based on the
# output of running various 3rd party tools meant to help with your
# build system. For example, it runs root-config --cflags and puts the
# results in the ROOTCFLAGS cmake variable. That variable is then used
# in generating the jana-config script so that it can report those flags
# for use when building against this version of JANA.
#
# In addition, some variables such as HAVE_ROOT are set which can be used
# in preprocessor directives to conditionally compile code depending on
# whether the 3rd party package is present.
#

set(JANA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

execute_process(COMMAND SBMS/osrelease.pl
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE BMS_OSNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE)

# ROOT
if(DEFINED ENV{ROOTSYS})
    set(HAVE_ROOT 1)
    set(ROOTSYS $ENV{ROOTSYS})

    execute_process(COMMAND $ENV{ROOTSYS}/bin/root-config --cflags
                    OUTPUT_VARIABLE ROOTCFLAGS
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND $ENV{ROOTSYS}/bin/root-config --glibs
                    OUTPUT_VARIABLE ROOTGLIBS
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
#    message(STATUS "Did not find ROOT")
    set(HAVE_ROOT 0)
endif()

# XERCESC
# n.b. this is hard-coded for now to assume XERCES 3
if(DEFINED ENV{XERCESCROOT})
    set(HAVE_XERCES 1)
    set(XERCES3 1)
    set(XERCESCROOT $ENV{XERCESCROOT})
    set(XERCES_CPPFLAGS "-I${XERCESCROOT}/include/xercesc")
    set(XERCES_LIBS "-lxerces-c")
    if( NOT $XERCESCROOT EQUAL "/usr" )
        set(XERCES_CPPFLAGS "${XERCES_CPPFLAGS} -I${XERCESCROOT}/include")
        set(XERCES_LDFLAGS "-L${XERCESCROOT}/lib")
    endif()
else()
    find_package(XercesC)
    if(XercesC_FOUND)
        set(HAVE_XERCES 1)
        set(XERCES3 1)
        get_filename_component(XERCESCROOT "${XercesC_INCLUDE_DIRS}" DIRECTORY)
        set(XERCES_CPPFLAGS "-I${XercesC_INCLUDE_DIRS} -I${XercesC_INCLUDE_DIRS}/xercesc")
        set(XERCES_LIBS "${XercesC_LIBRARIES}")
    endif()
endif()

# cMsg
# n.b. I don't believe this will be needed in JANA2
#if(DEFINED ENV{CMSGROOT})
#    set(HAVE_CMSG 1)
#    set(CMSG_CPPFLAGS "-I${CMSGROOT}/include")
#    set(CMSG_LDFLAGS "-L${CMSGROOT}/lib")
#    set(CMSG_LIBS "-lcmsg -lcmsgxx -lcmsgRegex")
#else()
#    set(HAVE_CMSG 0)
#endif()


# CCDB
if(DEFINED ENV{CCDB_HOME})
    set(HAVE_CCDB 1)
    set(CCDB_HOME $ENV{CCDB_HOME})
    set(CCDB_CPPFLAGS "-I${CCDB_HOME}/include")
    set(CCDB_LDFLAGS "-L${CCDB_HOME}/lib")
    set(CCDB_LIBS "-lccdb")
else()
    set(HAVE_CCDB 0)
endif()

# MySQL
# TODO: Strange that MySQL does not provide a FindMySQL.cmake. There seem to  be plenty out there though and we should maybe adopt one to replace the following
execute_process(COMMAND which mysql_config
                OUTPUT_VARIABLE MYSQL_FOUND
                OUTPUT_STRIP_TRAILING_WHITESPACE)
if(MYSQL_FOUND)
    set(HAVE_MYSQL 1)
    set(JANA2_MYSQL_CONFIG ${MYSQL_FOUND})
    execute_process(COMMAND mysql_config --cflags
            OUTPUT_VARIABLE MYSQL_CFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND mysql_config --libs
            OUTPUT_VARIABLE MYSQL_LDFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND mysql_config --version
            OUTPUT_VARIABLE MYSQL_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND mysql_config --variable=pkglibdir
            OUTPUT_VARIABLE MYSQL_PKGLIBDIR
            OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(HAVE_MYSQL 0)
endif()

# CURL
execute_process(COMMAND "which curl-config 2> /dev/null"
        OUTPUT_VARIABLE CURL_FOUND
        OUTPUT_STRIP_TRAILING_WHITESPACE)

if(CURL_FOUND)
    set(HAVE_CURL 1)
    set(JANA2_CURL_CONFIG ${CURL_FOUND})
    execute_process(COMMAND curl-config --cflags
            OUTPUT_VARIABLE CURL_CFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND curl-config --libs
            OUTPUT_VARIABLE CURL_LDFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND curl-config --prefix
            OUTPUT_VARIABLE CURL_ROOT
            OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(HAVE_CURL 0)
endif()

# TODO: FindNuma.cmake
set(HAVE_NUMA 0)

configure_file(scripts/jana-config.in jana-config @ONLY)
configure_file(scripts/jana_config.h.in jana_config.h @ONLY)
configure_file(scripts/jana-this.sh.in  jana-this.sh  @ONLY)
configure_file(scripts/jana-this.csh.in jana-this.csh  @ONLY)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jana-config DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jana_config.h DESTINATION include/JANA)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jana-this.sh  DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jana-this.csh  DESTINATION bin)
