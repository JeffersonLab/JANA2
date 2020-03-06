
set(JANA_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

execute_process(COMMAND SBMS/osrelease.pl
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE BMS_OSNAME
        OUTPUT_STRIP_TRAILING_WHITESPACE)


if(DEFINED ENV{ROOTSYS})
    set(HAVE_ROOT 1)

    execute_process(COMMAND $ENV{ROOTSYS}/bin/root-config --cflags
                    OUTPUT_VARIABLE ROOTCFLAGS
                    OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND $ENV{ROOTSYS}/bin/root-config --glibs
                    OUTPUT_VARIABLE ROOTGLIBS
                    OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    message(STATUS "Did not find ROOT")
    set(HAVE_ROOT 0)
endif()


if(DEFINED ENV{XERCESCROOT})
    set(HAVE_XERCES 1)
    set(XERCES_CPPFLAGS "-I${XERCESCROOT}/include -I${XERCESCROOT}/include/xercesc")
    set(XERCES_LDFLAGS "-L${XERCESCROOT}/lib")
    set(XERCES_LIBS "-lxerces-c")
else()
    # TODO: find_package(XERCES) just in case it was installed via a normal package manager instead
    set(HAVE_XERCES 0)
endif()


find_file(XERCES3_HEADER "xercesc/dom/DOMImplementationList.hpp"
          PATHS "$ENV{XERCESCROOT}/include" "$ENV{XERCESCROOT}/include/xercesc")

if(XERCES3_HEADER)
    set(XERCES3 1)
else()
    set(XERCES3 0)
endif()


if(DEFINED $ENV{CMSGROOT})
    set(HAVE_CMSG 1)
    set(CMSG_CPPFLAGS "-I${CMSGROOT}/include")
    set(CMSG_LDFLAGS "-L${CMSGROOT}/lib")
    set(CMSG_LIBS "-lcmsg -lcmsgxx -lcmsgRegex")
else()
    set(HAVE_CMSG 0)
endif()



if(DEFINED $ENV{CCDB_HOME})
    set(HAVE_CCDB 1)
    set(CCDB_CPPFLAGS "-I${CCDB_HOME}/include")
    set(CCDB_LDFLAGS "-L${CCDB_HOME}/lib")
    set(CCDB_LIBS "-lccdb")
else()
    set(HAVE_CCDB 0)
endif()


execute_process(COMMAND which mysql_config
                OUTPUT_VARIABLE MYSQL_FOUND
                OUTPUT_STRIP_TRAILING_WHITESPACE)
if(MYSQL_FOUND)
    set(HAVE_MYSQL 1)
    execute_process(COMMAND mysql_config --cflags
            OUTPUT_VARIABLE MYSQL_CFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND mysql_config --libs
            OUTPUT_VARIABLE MYSQL_LDFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND mysql_config --version
            OUTPUT_VARIABLE MYSQL_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(HAVE_MYSQL 0)
endif()


execute_process(COMMAND which curl-config
        OUTPUT_VARIABLE CURL_FOUND
        OUTPUT_STRIP_TRAILING_WHITESPACE)

if(CURL_FOUND)
    set(HAVE_CURL 1)

    execute_process(COMMAND curl-config --cflags
            OUTPUT_VARIABLE CURL_CFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE)

    execute_process(COMMAND curl-config --libs
            OUTPUT_VARIABLE CURL_LDFLAGS
            OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
    set(HAVE_CURL 0)
endif()

# TODO: FindNuma.cmake
set(HAVE_NUMA 0)

configure_file(scripts/jana-config.in jana-config @ONLY)
configure_file(scripts/jana_config.h.in jana_config.h @ONLY)

install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jana-config DESTINATION bin)
install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/jana_config.h DESTINATION include/JANA)
