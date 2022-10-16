

# Generate JVersion.h
# Note that we have to re-run cmake to force this to update
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

configure_file(JVersion.h.in JVersion.h)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/JVersion.h DESTINATION include/JANA/CLI)
