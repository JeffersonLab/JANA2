

add_jana_test(jana-perf-tests)

if (USE_PODIO)
    find_package(podio REQUIRED)
    target_link_libraries(jana-perf-tests PRIVATE PodioDatamodel PodioDatamodelDict)

else()
    message(STATUS "jana-perf-tests compiled without PODIO stress test because USE_PODIO=Off")

endif()

