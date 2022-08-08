set(PERF_TEST_SOURCES
        PerfTests.cc
        )

add_executable(jana-perf-tests ${PERF_TEST_SOURCES})
find_package(Threads REQUIRED)
target_include_directories(jana-perf-tests PUBLIC .)
target_link_libraries(jana-perf-tests jana2 Threads::Threads)

install(TARGETS jana-perf-tests DESTINATION bin)
