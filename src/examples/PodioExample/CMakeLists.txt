
set(PodioExample_SOURCES
        PodioExample.cc
        )

if (USE_PODIO)
    find_package(podio REQUIRED)
    add_executable(PodioExample ${PodioExample_SOURCES})
    target_link_libraries(PodioExample jana2 podio::podio)
    install(TARGETS PodioExample DESTINATION bin)
else()
    message(STATUS "Skipping examples/PodioExample because USE_PODIO=Off")

endif()



