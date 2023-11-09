set(PERF_TEST_SOURCES
        PerfTests.cc
        )

add_executable(jana-perf-tests ${PERF_TEST_SOURCES})
find_package(Threads REQUIRED)
target_include_directories(jana-perf-tests PUBLIC .)
target_link_libraries(jana-perf-tests jana2 Threads::Threads)


if (USE_PODIO)
    find_package(podio REQUIRED)
    target_link_libraries(jana-perf-tests podio::podio PodioExampleDatamodel PodioExampleDatamodelDict podio::podioRootIO)
    set_target_properties(jana-perf-tests PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

else()
    message(STATUS "jana-perf-tests compiled without PODIO stress test because USE_PODIO=Off")

endif()



install(TARGETS jana-perf-tests DESTINATION bin)
