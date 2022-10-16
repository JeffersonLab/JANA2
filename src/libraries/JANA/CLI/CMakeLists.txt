

# Generate JVersion.h

# Force CMake to reconfigure itself whenever git's HEAD pointer changes
set_property(
        DIRECTORY
        APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/.git/HEAD
)

execute_process(
        COMMAND git log -1 --format=%H
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE JVERSION_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND git log -1 --format=%aD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE JVERSION_COMMIT_DATE
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
        COMMAND git show-ref -s v${jana2_VERSION_MAJOR}.${jana2_VERSION_MINOR}.${jana2_VERSION_PATCH}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE JVERSION_RELEASE_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(JVERSION_RELEASE_COMMIT_HASH EQUAL JVERSION_COMMIT_HASH)
    set(JVERSION_RELEASE 1)
else()
    set(JVERSION_RELEASE 0)
endif()

execute_process(
        COMMAND git status --porcelain
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE JVERSION_MODIFIED_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(JVERSION_MODIFIED_FILES)
    set(JVERSION_MODIFIED 1)
else()
    set(JVERSION_MODIFIED 0)
endif()

configure_file(JVersion.h.in JVersion.h)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/JVersion.h DESTINATION include/JANA/CLI)
