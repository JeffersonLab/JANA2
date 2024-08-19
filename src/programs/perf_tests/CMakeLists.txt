

add_jana_test(jana-perf-tests)

if (USE_PODIO)
    find_package(podio REQUIRED)
    target_link_libraries(jana-perf-tests PRIVATE podio::podio PodioExampleDatamodel PodioExampleDatamodelDict podio::podioRootIO)

else()
    message(STATUS "jana-perf-tests compiled without PODIO stress test because USE_PODIO=Off")

endif()

