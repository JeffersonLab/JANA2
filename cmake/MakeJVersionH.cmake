

# Generate JVersion.h

# Force CMake to reconfigure itself whenever git's HEAD pointer changes
# Note that HEAD usually points to the branch name, not the commit hash.
# This will correctly force CMake to reconfigure itself when you check out an existing branch, tag, or commit.
# It will not force CMake to reconfigure if you are adding successive commits to a branch.
# For that, you have to manually reconfigure CMake.
set_property(
        DIRECTORY
        APPEND
        PROPERTY CMAKE_CONFIGURE_DEPENDS
        ${CMAKE_SOURCE_DIR}/.git/HEAD
)

execute_process(
        COMMAND git log -1 --format=%H
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        RESULT_VARIABLE JVERSION_GIT_RESULT
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

execute_process(
        COMMAND git status --porcelain
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE JVERSION_MODIFIED_FILES
        OUTPUT_STRIP_TRAILING_WHITESPACE
)


# If our call to git log returned a non-zero exit code, we don't have git information and shouldn't pretend we do
if(JVERSION_GIT_RESULT EQUAL 0)
    set(JVERSION_UNKNOWN 0)
else()
    set(JVERSION_UNKNOWN 1)
endif()

# We figure out whether we are actually the release commit (according to git) by comparing the commit hashes
if(JVERSION_RELEASE_COMMIT_HASH STREQUAL JVERSION_COMMIT_HASH)
    set(JVERSION_RELEASE 1)
else()
    set(JVERSION_RELEASE 0)
endif()

# Even if `git log` indicates we are on the latest release, there might have been some changes
# made on top that haven't been committed yet
#
if(JVERSION_MODIFIED_FILES STREQUAL "")
    set(JVERSION_MODIFIED 0)
else()
    set(JVERSION_MODIFIED 1)
endif()

message(STATUS "Generating JVersion.h")

configure_file(src/libraries/JANA/CLI/JVersion.h.in ${CMAKE_CURRENT_BINARY_DIR}/src/libraries/JANA/CLI/JVersion.h @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/src/libraries/JANA/CLI/JVersion.h DESTINATION include/JANA/CLI)
