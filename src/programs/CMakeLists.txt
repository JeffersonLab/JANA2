
add_subdirectory(jana)

if (${BUILD_TESTS})
    add_subdirectory(unit_tests)
    add_subdirectory(integration_tests)
    add_subdirectory(perf_tests)
endif()


